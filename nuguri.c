#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h> //Sleep
    #include <conio.h> //kbhit, getch
#else
    #include <unistd.h> // usleep
    #include <termios.h>
    #include <fcntl.h>
#endif
#include <time.h>

// 맵 및 게임 요소 정의 (동적 크기 지원)
typedef struct
{
    int width;
    int height;
    char** rows;
} Stage;

// 구조체 정의
typedef struct
{
    int x, y;
    int dir; // 1: right, -1: left
} Enemy;

typedef struct
{
    int x, y;
    int collected;
} Coin;

// 전역 변수
Stage* stages = NULL; // map.txt에서 읽어 만든 스테이지 목록
int stage_count = 0; // 실제로 로드된 스테이지 개수
int player_x, player_y;
int spawn_x, spawn_y; // 스테이지 내 출발 지점(S)
int stage = 0;
int score = 0;
const int MAX_HEALTH = 3;

// 플레이어 상태
int is_jumping = 0;
int velocity_y = 0;
int on_ladder = 0;
int health = 3;

// 게임 객체
Enemy* enemies = NULL; // 가변 길이 적 배열
int enemy_count = 0;
int enemy_capacity = 0; // 현재 할당된 적 배열 크기
Coin* coins = NULL; // 가변 길이 코인 배열
int coin_count = 0;
int coin_capacity = 0; // 현재 할당된 코인 배열 크기

// 화면 버퍼 (스테이지 크기 변경 시에만 재할당)
char** display_rows = NULL;
char* display_buffer = NULL;
int display_width = 0;
int display_height = 0;

// 터미널 설정
#ifndef _WIN32
struct termios orig_termios;
#endif

// 함수 선언
void disable_raw_mode();
void enable_raw_mode();
// 임시로 모은 한 스테이지의 행들을 Stage 구조체로 묶어 stages 배열에 추가
void append_stage(char** temp_lines, int temp_count, int max_width);
void load_maps();
void init_stage();
void draw_game();
void update_game(char input);
void move_player(char input);
void move_enemies();
void check_collisions();
int kbhit();
void textcolor(int color);
void health_system();
void draw_health();
void opening(void);
void ending(void);
void game_over(void);
void cls_screen(void);
void void_screen();
void hide_cursor(void);
void cls_mem();
void beep();
void delay(int ms);
int getch();

int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #endif
    void_screen();
    hide_cursor();
    opening();
    void_screen();
    srand(time(NULL));
    // 맵을 동적으로 읽어 stage_count와 stages를 세팅한 뒤 게임 루프 실행
    #ifndef _WIN32
    enable_raw_mode();
    #endif
    atexit(cls_mem);
    load_maps();
    init_stage();

    char c = '\0';
    int game_over = 0;

    while (!game_over && stage < stage_count)
    {
        if (kbhit())
        {
            c = getch();
            if (c == 'q')
            {
                game_over = 1;
                continue;
            }
            if (c == '\x1b')
            {
                getch(); // '['
                switch (getch())
                {
                case 'A': c = 'w';
                    break; // Up
                case 'B': c = 's';
                    break; // Down
                case 'C': c = 'd';
                    break; // Right
                case 'D': c = 'a';
                    break; // Left
                }
            }
        }
        else
        {
            c = '\0';
        }

        // 남은 입력을 즉시 비워 입력 버퍼가 쌓이는 것을 방지
        while (kbhit()) { getch(); }

        update_game(c);
        draw_game();
        #ifdef _WIN32
            delay(30); // 윈도우에서는 더 빠른 속도
        #else
            delay(90); // 다른 운영체제에서는 기존 속도
        #endif

        if (stages[stage].rows[player_y][player_x] == 'E')
        {
            stage++;
            score += 100;
            if (stage < stage_count)
            {
                init_stage();
            }
            else
            {
                game_over = 1;
                cls_screen();
                ending();
                printf("축하합니다! 모든 스테이지를 클리어했습니다!\n");
                printf("최종 점수: %d\n", score);
            }
        }
    }

    #ifndef _WIN32
    disable_raw_mode();
    #endif
    return 0;
}


// 터미널 Raw 모드 활성화/비활성화
#ifndef _WIN32
void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
#endif

// 임시로 모은 한 스테이지의 행들을 Stage 구조체로 묶어 stages 배열에 추가
void append_stage(char** temp_lines, int temp_count, int max_width)
{
    if (temp_count == 0) return;

    // 하나의 스테이지를 동적 할당: 높이는 읽은 행 수, 폭은 가장 긴 행 길이
    Stage st;
    st.width = max_width;
    st.height = temp_count;
    st.rows = (char**)malloc(sizeof(char*) * st.height);
    if (!st.rows)
    {
        fprintf(stderr, "스테이지 메모리 할당 실패\n");
        exit(1);
    }

    for (int i = 0; i < temp_count; i++)
    {
        st.rows[i] = (char*)malloc(st.width + 1);
        if (!st.rows[i])
        {
            fprintf(stderr, "스테이지 행 메모리 할당 실패\n");
            exit(1);
        }
        memset(st.rows[i], ' ', st.width);
        int len = (int)strlen(temp_lines[i]);
        if (len > st.width) len = st.width;
        memcpy(st.rows[i], temp_lines[i], len);
        st.rows[i][st.width] = '\0';
        free(temp_lines[i]);
    }
    free(temp_lines);

    Stage* new_stages = (Stage*)malloc(sizeof(Stage) * (stage_count + 1));
    if (!new_stages)
    {
        fprintf(stderr, "스테이지 확장 실패\n");
        exit(1);
    }
    if (stages && stage_count > 0)
        memcpy(new_stages, stages, sizeof(Stage) * stage_count);
    free(stages);
    stages = new_stages;
    stages[stage_count++] = st;
}

// 맵 파일 로드
void load_maps()
{
    // map.txt를 읽어 빈 줄로 스테이지를 구분하고, 각 스테이지를 동적 Stage로 변환
    FILE* file = fopen("map.txt", "r");
    if (!file)
    {
        fprintf(stderr, "map.txt 파일을 열 수 없습니다\n");
        exit(1);
    }

    char** temp_lines = NULL;
    int temp_capacity = 0; // 읽은 행 버퍼 크기 (가변 확장)
    int temp_count = 0;
    int max_width = 0; // 현재 스테이지의 최대 행 길이
    char line[1024];

    while (fgets(line, sizeof(line), file))
    {
        size_t len = strcspn(line, "\n\r");
        line[len] = '\0';

        if (len == 0)
        {
            if (temp_count > 0)
            {
                append_stage(temp_lines, temp_count, max_width);
                temp_lines = NULL;
                temp_capacity = 0;
                temp_count = 0;
                max_width = 0;
            }
            continue;
        }

        if (temp_count >= temp_capacity)
        {
            int new_capacity = temp_capacity ? temp_capacity * 2 : 32; // 점진적 2배 확장으로 realloc 대체
            char** new_lines = (char**)malloc(sizeof(char*) * new_capacity);
            if (!new_lines)
            {
                fprintf(stderr, "맵 라인 버퍼 확장 실패\n");
                exit(1);
            }
            if (temp_lines && temp_capacity > 0)
                memcpy(new_lines, temp_lines, sizeof(char*) * temp_capacity);
            free(temp_lines);
            temp_lines = new_lines;
            temp_capacity = new_capacity;
        }

        temp_lines[temp_count] = (char*)malloc(len + 1);
        if (!temp_lines[temp_count])
        {
            fprintf(stderr, "맵 라인 메모리 할당 실패\n");
            exit(1);
        }
        memcpy(temp_lines[temp_count], line, len + 1);
        if ((int)len > max_width) max_width = (int)len;
        temp_count++;
    }
    fclose(file);

    if (temp_count > 0)
    {
        append_stage(temp_lines, temp_count, max_width);
        temp_lines = NULL;
    }

    if (temp_lines)
    {
        for (int i = 0; i < temp_count; i++) free(temp_lines[i]);
        free(temp_lines);
    }

    if (stage_count == 0)
    {
        fprintf(stderr, "map.txt에 유효한 스테이지가 없습니다.\n");
        exit(1);
    }
}
// 동적 할당한 stages, enemy, coins 객체들의 메모리를 해제하는 함수. atexit(cls_mem())처럼 사용
void cls_mem()
{
    if (stages)
    {
        for (int i = 0; i < stage_count; i++)
        {
            if (stages[i].rows)
            {
                for (int y = 0; y < stages[i].height; y++)
                {
                    free(stages[i].rows[y]);
                }
                free(stages[i].rows);
                stages[i].rows = NULL;
            }
        }
        free(stages);
        stages = NULL;
    }
    //stage_count = 0;

    free(enemies);
    enemies = NULL;
    //enemy_count = 0;
    //enemy_capacity = 0;

    free(coins);
    coins = NULL;
    //coin_count = 0;
    //coin_capacity = 0;

    free(display_buffer);
    display_buffer = NULL;
    //display_width = 0;
    //display_height = 0;

    free(display_rows);
    display_rows = NULL;
}


// 현재 스테이지 초기화
void init_stage()
{
    enemy_count = 0;
    coin_count = 0;
    is_jumping = 0;
    velocity_y = 0;

    Stage* st = &stages[stage];

    if (st->width != display_width || st->height != display_height)
    {
        free(display_buffer);
        free(display_rows);

        display_width = st->width;
        display_height = st->height;

        display_buffer = (char*)malloc(display_width * display_height);
        display_rows = (char**)malloc(sizeof(char*) * display_height);
        if (!display_buffer || !display_rows)
        {
            fprintf(stderr, "화면 버퍼 할당 실패\n");
            exit(1);
        }
        for (int i = 0; i < display_height; i++)
        {
            display_rows[i] = display_buffer + (i * display_width);
        }
    }

    if (display_buffer)
    {
        memset(display_buffer, ' ', display_width * display_height);
    }

    for (int y = 0; y < st->height; y++)
    {
        for (int x = 0; x < st->width; x++)
        {
            char cell = st->rows[y][x];
            if (cell == 'S')
            {
                player_x = x;
                player_y = y;
                spawn_x = x; // S 위치 기록용(S는 한번 사용한 후에 stage에서 사라짐)
                spawn_y = y;
            }
            else if (cell == 'X')
            {
                // 적 배열이 가득 차면 2배씩 확장 (0->8로 시작 후 2배씩 키워 재할당 횟수를 줄인다)
                // realloc 대신 malloc+memcpy로 새 버퍼를 만들어 교체
                if (enemy_count >= enemy_capacity)
                {
                    int new_cap = enemy_capacity ? enemy_capacity * 2 : 8;
                    Enemy* new_enemies = (Enemy*)malloc(sizeof(Enemy) * new_cap);
                    if (!new_enemies)
                    {
                        fprintf(stderr, "적 메모리 확장 실패\n");
                        exit(1);
                    }
                    if (enemies && enemy_capacity > 0)
                        memcpy(new_enemies, enemies, sizeof(Enemy) * enemy_capacity);
                    free(enemies);
                    enemies = new_enemies;
                    enemy_capacity = new_cap;
                }
                enemies[enemy_count] = (Enemy){x, y, (rand() % 2) * 2 - 1};
                enemy_count++;
            }
            else if (cell == 'C')
            {
                // 코인 배열도 동일하게 가변 확장 (초기 8개, 이후 2배씩)
                // realloc 없이 새 버퍼를 할당하고 이전 내용을 복사
                if (coin_count >= coin_capacity)
                {
                    int new_cap = coin_capacity ? coin_capacity * 2 : 8;
                    Coin* new_coins = (Coin*)malloc(sizeof(Coin) * new_cap);
                    if (!new_coins)
                    {
                        fprintf(stderr, "코인 메모리 확장 실패\n");
                        exit(1);
                    }
                    if (coins && coin_capacity > 0)
                        memcpy(new_coins, coins, sizeof(Coin) * coin_capacity);
                    free(coins);
                    coins = new_coins;
                    coin_capacity = new_cap;
                }
                coins[coin_count++] = (Coin){x, y, 0};
            }
        }
    }
}

// 게임 화면 그리기
void draw_game()
{
    Stage* st = &stages[stage];
    cls_screen();
    printf("Stage: %d/%d | Score: %d\n", stage + 1, stage_count, score);
    printf("조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");
    draw_health(); //체력 표시 함수 호출

    memset(display_buffer, ' ', display_width * display_height);

    for (int i = 0; i < coin_count; i++)
    {
        if (!coins[i].collected)
        {
            display_rows[coins[i].y][coins[i].x] = 'C';
        }
    }

    for (int i = 0; i < enemy_count; i++)
    {
        display_rows[enemies[i].y][enemies[i].x] = 'X';
    }

    display_rows[player_y][player_x] = 'P';

    for (int y = 0; y < st->height; y++)
    {
        for (int x = 0; x < st->width; x++)
        {
            char object_cell = display_rows[y][x];
            char base_cell = st->rows[y][x];
            char map_cell = (base_cell == 'S' || base_cell == 'X' || base_cell == 'C') ? ' ' : base_cell;
            char cell = (object_cell != ' ') ? object_cell : map_cell;
            
            switch (cell)
            {
                case '#':
                    textcolor(8); //회색
                    break;
                case 'H':
                    textcolor(6); //청록색
                    break;
                case 'C':
                    textcolor(3); //노란색
                    break;
                case 'X':
                    textcolor(1); //빨간색
                    break;
                case 'P':
                    textcolor(2); //초록색
                    break;
                default:
                    textcolor(9); //기본색
                    break;
            }
            printf("%c", cell);
            textcolor(9);
        }
        printf("\n");
    }
}

// 게임 상태 업데이트
void update_game(char input)
{
    move_player(input);
    move_enemies();
    check_collisions();
}

// 플레이어 이동 로직 전반적인 수정
void move_player(char input)
{
    Stage* st = &stages[stage];
    char floor_tile = 0;
    char current_tile = 0;
    int next_x = player_x;

    // 수평 이동 처리
    switch (input)
    {
        case 'a': next_x--; break;
        case 'd': next_x++; break;
    }
    
    if (next_x >= 0 && next_x < st->width && st->rows[player_y][next_x] != '#')
    {
        player_x = next_x;
    }

    // 현재 위치 정보 갱신
    floor_tile = (player_y + 1 < st->height) ? st->rows[player_y + 1][player_x] : '#';
    current_tile = st->rows[player_y][player_x];
    
    // 사다리 판정 갱신
    on_ladder = (current_tile == 'H');

    // 사다리 끝(위가 '#')에서 점프 시 천장 위로 올라감.
    if (input == ' ' && !is_jumping) 
    {
        int climbed = 0;
        if (on_ladder && player_y > 0 && st->rows[player_y - 1][player_x] == '#')
        {
            int climb_y = player_y - 1;
            while (climb_y >= 0 && st->rows[climb_y][player_x] == '#') climb_y--;
            if (climb_y >= 0 && st->rows[climb_y][player_x] != '#')
            {
                player_y = climb_y;
                floor_tile = (player_y + 1 < st->height) ? st->rows[player_y + 1][player_x] : '#';
                current_tile = st->rows[player_y][player_x];
                on_ladder = (current_tile == 'H');
                climbed = 1;
            }
        }

        // 사다리에 붙어 있거나 바닥 위면 점프. 단, 방금 천장 위로 올라섰을 때는 점프 생략.
        if (!climbed && (floor_tile == '#' || on_ladder)) 
        {
            is_jumping = 1;
            velocity_y = -2;
        }
    }

    // 사다리 이동 처리 (점프 중이 아닐 때만 고정)
    // (!is_jumping) 조건을 추가하여 점프 중일 때는 사다리 로직 무시
    if (on_ladder && !is_jumping)
    {
        velocity_y = 0; // 중력 무시
        if (input == 'w') 
        {
            if (player_y - 1 >= 0 && st->rows[player_y - 1][player_x] != '#')
                player_y--;
        }
        else if (input == 's')
        {
            if (player_y + 1 < st->height && st->rows[player_y + 1][player_x] != '#')
                player_y++;
        }
    }
    else 
    {
        // 지상/공중 물리 처리 (중력 및 점프)
        
        // 걷다가 낭떠러지로 떨어진 경우 (점프도 아니고 사다리도 아님)
        if (!is_jumping && floor_tile == ' ' && !on_ladder)
        {
            is_jumping = 1;
            velocity_y = 1; // 낙하 시작
        }

        if (is_jumping)
        {
            int steps = abs(velocity_y); 
            int dir = (velocity_y > 0) ? 1 : -1; 

            for (int i = 0; i < steps; i++)
            {
                int test_y = player_y + dir;

                // 맵 범위 체크
                if (test_y < 0 || test_y >= st->height)
                {
                    if (test_y >= st->height) init_stage();
                    velocity_y = 0;
                    break;
                }

                char target_cell = st->rows[test_y][player_x];

                // 벽 충돌 체크
                if (target_cell == '#')
                {
                    velocity_y = 0;
                    if (dir == 1) // 바닥 착지
                    {
                        is_jumping = 0;
                    }
                    break; 
                }
                
                // 이동 확정
                player_y = test_y;
            }

            // 중력 적용
            if (is_jumping) 
            {
                velocity_y++;
                if(velocity_y > 3) velocity_y = 3;
            }
        }
    }

    // 맵 밖으로 나갔는지 최종 확인
    if (player_y >= st->height) init_stage();
}

// 적 이동 로직
void move_enemies()
{
    Stage* st = &stages[stage]; // 동적 폭/높이를 사용해 AI 경계 체크
    for (int i = 0; i < enemy_count; i++)
    {
        int next_x = enemies[i].x + enemies[i].dir;
        int y = enemies[i].y;
        int out_of_bounds = (next_x < 0 || next_x >= st->width);
        int hit_wall = (!out_of_bounds && st->rows[y][next_x] == '#');
        int gap_ahead = (y + 1 >= st->height) || (!out_of_bounds && st->rows[y + 1][next_x] == ' ');

        if (out_of_bounds || hit_wall || gap_ahead)
        {
            enemies[i].dir *= -1;
        }
        else
        {
            enemies[i].x = next_x;
        }
    }
}

// 충돌 감지 로직
void check_collisions()
{
    for (int i = 0; i < enemy_count; i++)
    {
        if (player_x == enemies[i].x && player_y == enemies[i].y)
        {
            score = (score > 50) ? score - 50 : 0;
            health_system(); //적과 충돌 시 생명력 감소
            player_x = spawn_x; // 처음 위치로 플레이어 이동
            player_y = spawn_y;
            is_jumping = 0; // 이동 로직 초기화
            velocity_y = 0;
            return;
        }
    }
    for (int i = 0; i < coin_count; i++)
    {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y)
        {
            coins[i].collected = 1;
            score += 20;
            beep();
        }
    }
}

// 비동기 키보드 입력 확인
int kbhit()
{
    #ifdef _WIN32
        return _kbhit();
    #else
        struct termios oldt, newt;
        int ch;
        int oldf;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if (ch != EOF)
        {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    #endif
}

int getch()
{
    #ifdef _WIN32
        return _getch(); //엔터 키 없이 입력 반환
    #else
        return getchar();
    #endif
}

void textcolor(int color)
{
    // 색깔바꾸기
    if (color < 20)
    {
        printf("\033[%dm", color + 30); //텍스트의 전경색 계산
    }
    else
    {
        printf("\033[%dm", color + 70); //텍스트의 밝은 전경색 계산
    }
    // Black	0	10
    // Red	    1	11
    // Green	2	12
    // Yellow	3	13
    // Blue	    4	14
    // Magenta	5	15
    // Cyan	    6	16
    // White	7	17
    // Default	9	19

    // Bright Black	20	30
    // Bright Red	21	31
    // Bright Green	22	32
    // Bright Yellow23	33
    // Bright Blue	24	34
    // Bright Magenta   25	35
    // Bright Cyan	26	36
    // Bright White	27	37
}

//체력을 감소 시키고 게임 오버 여부 확인
void health_system()
{
    health--;
    if (health <= 0) //체력 소진 시 게임 오버
    {
        void_screen();
        game_over();
    }
}

//현재 체력 상태를 하트 기호로 출력
void draw_health()
{
    for(int i = 0; i < health; i++) //남은 체력만큼 하트 출력
    { 
        textcolor(1);
        printf("♥ ");
    }
    for(int i = 0; i < MAX_HEALTH-health; i++) //깎인 체력만큼 빈 하트 출력
    { 
        textcolor(1);
        printf("♡ ");
    }
    printf("\n");
    textcolor(9);
}

void opening(void)
{
    cls_screen(); // 화면 지우기
    delay(200);

    const char* frames[] = {
        " \n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n",

        " \n"
        "       ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        "        (\\_/) \n"
        "        ( •_•) \n"
        "        / >🍒  \n",

        " \n"
        "       ██║╚██╗██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "       ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\n"
        "        ( •_•)\t( •_•)\n"
        "        / >🍒\t/ >🍒\n",


        " \n"
        "       ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██████╔╝██║\n"
        "       ██║╚██╗██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "       ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/) \n"
        "        ( •_•)\t( •_•)\t( •_•) \n"
        "        / >🍒\t/ >🍒\t/ >🍒\n",

        " \n"
        "       ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔══██╗██║\n"
        "       ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██████╔╝██║\n"
        "       ██║╚██╗██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "       ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\n"
        "        ( •_•)\t( •_•)\t( •_•)\t( •_•)\n"
        "        / >🍒\t/ >🍒\t/ >🍒\t/ >🍒\n",

        " \n"
        "       ███╗   ██╗██╗   ██╗ ██████╗ ██╗   ██╗██████╗ ██╗\n"
        "       ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔══██╗██║\n"
        "       ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██████╔╝██║\n"
        "       ██║╚██╗██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "       ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\t(\\_/) \n"
        "        ( •_•)\t( •_•)\t( •_•)\t( •_•)\t( •_•) \n"
        "        / >🍒\t/ >🍒\t/ >🍒\t/ >🍒\t/ >🍒  \n",

        " \n"
        "                                                              \n"
        "       ███╗   ██╗██╗   ██╗ ██████╗ ██╗   ██╗██████╗ ██╗\n"
        "       ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔══██╗██║\n"
        "       ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██████╔╝██║\n"
        "       ██║╚██╗██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "       ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "       ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\t(\\_/) \n"
        "        ( •_•)\t( •_•)\t( •_•)\t( •_•)\t( •_•)\t( •_•) \n"
        "        / >🍒\t/ >🍒\t/ >🍒\t/ >🍒\t/ >🍒\t/ >🍒  \n"

        //
        // " \n"
        // "     ███╗   ██╗██╗   ██╗ ██████╗ ██╗   ██╗██████╗ ██╗\n"
        // "     ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔══██╗██║\n"
        // "     ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██████╔╝██║\n"
        // "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        // "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        // "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n",
    };

    int frame_count = sizeof(frames) / sizeof(frames[0]);

    for (int i = 0; i < frame_count; i++)
    {
        cls_screen();
        printf("%s\n", frames[i]);
        delay(500);
    }

    printf("\n계속 진행하려면 엔터..\n");
    getchar();
}

void ending(void)
{
    const char* frames[] = {
        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( ^_^ )\t ( ^_^ )\n"
        "        / >🍒\t / >🍒 \n",

        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( ^o^ )\t ( ^o^ )\n"
        "        / >🍒\t / >🍒 \n",

        "\n"
        "        (\\_/)\t  (\\_/)\n"
        "      \\( ^o^ )/\t\\( ^o^ )/\n"
        "        /  🍒\t /  🍒 \n"
        "\n",

        "\n\n"
        "        (\\_/)\t (\\_/)\n"
        "       ( ^o^ )\t( ^o^ )\n"
        "        / >🍒\t/ >🍒 \n",

        "\n"
        "        (\\_/)\t  (\\_/)\n"
        "      \\( ^o^ )/\t\\( ^o^ )/\n"
        "        /  🍒\t /  🍒 \n"
        "\n",

        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( ^o^ )\t ( ^o^ )\n"
        "        / >🍒\t / >🍒 \n",


        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( -_- )\t ( -_- )\n"
        "       <  🍒\\\t <  🍒\\\n",

        "\n\n"
        "     ███████╗███╗   ██╗██████╗ \n"
        "     ██╔════╝████╗  ██║██╔══██╗\n"
        "     █████╗  ██╔██╗ ██║██║  ██║\n"
        "     ██╔══╝  ██║╚██╗██║██║  ██║\n"
        "     ███████╗██║ ╚████║██████╔╝\n"
        "     ╚══════╝╚═╝  ╚═══╝╚═════╝ \n"
        "\n"
    };

    int count = sizeof(frames) / sizeof(frames[0]);

    // while 문으로 교체
    int i = 0;
    while (1) {
        void_screen();  // 화면 클
        printf("%s\n", frames[count - 1]); // END
        printf("%s\n", frames[i]);    // 애니메이션 프레임
        printf("\n종료하려면 아무키나 입력...\n");
        delay(500);
        i = (i + 1) % (count-1);  // 프레임 순환
        // 엔터 키 입력 시 종료
        if (kbhit())
        {
            break;
        }
    }
}

void game_over(void)
{
    const char* frames[] = {
        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( T_T )\t ( T_T )\n"
        "        / >💧\t / >💧 \n",
        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( T^T )\t ( T^T )\n"
        "        /💧<\\\t  /💧<\\\n",
        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( T_T )\t ( T_T )\n"
        "        / >💧\t / >💧 \n",
        "\n\n"
        "        (\\_/)\t  (\\_/)\n"
        "       ( T^T )\t ( T^T )\n"
        "        /💧<\\\t  /💧<\\\n",
        
        "\n\n"
        "    ██████╗  █████╗ ███╗   ███╗███████╗\n"
        "   ██╔════╝ ██╔══██╗████╗ ████║██╔════╝\n"
        "   ██║  ███╗███████║██╔████╔██║█████╗  \n"
        "   ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝  \n"
        "   ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗\n"
        "    ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝\n"
        "    ██████╗ ██╗   ██╗███████╗██████╗ \n"
        "   ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n"
        "   ██║   ██║██║   ██║█████╗  ██████╔╝\n"
        "   ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n"
        "   ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n"
        "    ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n"
        "\n"
        "            --- E X I T ---            \n"
    };

    int frame_count = sizeof(frames) / sizeof(frames[0]);

    for (int i = 0; i < frame_count; i++)
    {
        cls_screen();
        printf("%s\n", frames[i]);
        delay(500);
    }
    exit(0);
}

void void_screen()
{
    // 전체 화면을 지우고 커서를 0,0으로 이동
    printf("\x1b[2J\x1b[H");
}

void cls_screen(void)
{
    // 커서만 0,0 으로 이동
    printf("\x1b[H");
}

void hide_cursor(void)
{
    //커서 숨기기
    printf("\x1b[?25l");
}

void beep()
{
    printf("\a"); // 비프음 발생
}

void delay(int ms)
{
    #ifdef _WIN32
        Sleep(ms);
    #else
        usleep(ms * 1000);
    #endif
}
