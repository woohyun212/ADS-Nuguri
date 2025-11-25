#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // usleep
#include <termios.h>
#include <fcntl.h>
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

// 터미널 설정
struct termios orig_termios;

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
void ending(void);

int main()
{
    srand(time(NULL));
    // 맵을 동적으로 읽어 stage_count와 stages를 세팅한 뒤 게임 루프 실행
    enable_raw_mode();
    load_maps();
    init_stage();

    char c = '\0';
    int game_over = 0;

    while (!game_over && stage < stage_count)
    {
        if (kbhit())
        {
            c = getchar();
            if (c == 'q')
            {
                game_over = 1;
                continue;
            }
            if (c == '\x1b')
            {
                getchar(); // '['
                switch (getchar())
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

        update_game(c);
        draw_game();
        usleep(90000);

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
                printf("\x1b[2J\x1b[H");
                printf("축하합니다! 모든 스테이지를 클리어했습니다!\n");
                printf("최종 점수: %d\n", score);
            }
        }
    }

    disable_raw_mode();
    return 0;
}


// 터미널 Raw 모드 활성화/비활성화
void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

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


// 현재 스테이지 초기화
void init_stage()
{
    enemy_count = 0;
    coin_count = 0;
    is_jumping = 0;
    velocity_y = 0;

    Stage* st = &stages[stage];

    for (int y = 0; y < st->height; y++)
    {
        for (int x = 0; x < st->width; x++)
        {
            char cell = st->rows[y][x];
            if (cell == 'S')
            {
                player_x = x;
                player_y = y;
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
    printf("\x1b[2J\x1b[H");
    printf("Stage: %d/%d | Score: %d\n", stage + 1, stage_count, score);
    printf("조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");
    draw_health(); //체력 표시 함수 호출

    // 동적 크기의 스테이지를 그리기 위해 매 프레임 임시 버퍼를 동적 할당해 사용 후 즉시 해제
    char** display_map = (char**)malloc(sizeof(char*) * st->height);
    if (!display_map)
    {
        fprintf(stderr, "화면 버퍼 할당 실패\n");
        exit(1);
    }

    for (int y = 0; y < st->height; y++)
    {
        display_map[y] = (char*)malloc(st->width + 1);
        if (!display_map[y])
        {
            fprintf(stderr, "화면 행 할당 실패\n");
            exit(1);
        }
        for (int x = 0; x < st->width; x++)
        {
            char cell = st->rows[y][x];
            if (cell == 'S' || cell == 'X' || cell == 'C')
            {
                display_map[y][x] = ' ';
            }
            else
            {
                display_map[y][x] = cell;
            }
        }
        display_map[y][st->width] = '\0';
    }

    for (int i = 0; i < coin_count; i++)
    {
        if (!coins[i].collected)
        {
            display_map[coins[i].y][coins[i].x] = 'C';
        }
    }

    for (int i = 0; i < enemy_count; i++)
    {
        display_map[enemies[i].y][enemies[i].x] = 'X';
    }

    display_map[player_y][player_x] = 'P';

    for (int y = 0; y < st->height; y++)
    {
        for (int x = 0; x < st->width; x++)
        {
            printf("%c", display_map[y][x]);
        }
        printf("\n");
        free(display_map[y]);
    }
    free(display_map);
}

// 게임 상태 업데이트
void update_game(char input)
{
    move_player(input);
    move_enemies();
    check_collisions();
}

// 플레이어 이동 로직
void move_player(char input)
{
    Stage* st = &stages[stage]; // 현재 스테이지의 동적 폭/높이/지면을 참조
    int next_x = player_x, next_y = player_y;
    char floor_tile = (player_y + 1 < st->height) ? st->rows[player_y + 1][player_x] : '#'; // 맵 아래면 낙하 처리 (동적 높이)
    char current_tile = st->rows[player_y][player_x];

    on_ladder = (current_tile == 'H');

    switch (input)
    {
    case 'a': next_x--;
        break;
    case 'd': next_x++;
        break;
    case 'w': if (on_ladder) next_y--;
        break;
    case 's': if (on_ladder && (player_y + 1 < st->height) && st->rows[player_y + 1][player_x] != '#') next_y++;
        break;
    case ' ':
        if (!is_jumping && (floor_tile == '#' || on_ladder))
        {
            is_jumping = 1;
            velocity_y = -2;
        }
        break;
    }

    // 동적 폭 기준으로 좌우 이동 가능 여부 확인
    if (next_x >= 0 && next_x < st->width && st->rows[player_y][next_x] != '#') player_x = next_x;

    if (on_ladder && (input == 'w' || input == 's'))
    {
        // 동적 높이 기준으로 사다리 이동 처리
        if (next_y >= 0 && next_y < st->height && st->rows[next_y][player_x] != '#')
        {
            player_y = next_y;
            is_jumping = 0;
            velocity_y = 0;
        }
    }
    else
    {
        if (is_jumping)
        {
            next_y = player_y + velocity_y;
            if (next_y < 0) next_y = 0;
            velocity_y++;

            // 위로 이동 시 천장을 만나면 속도 초기화
            if (velocity_y < 0 && next_y < st->height && st->rows[next_y][player_x] == '#')
            {
                velocity_y = 0;
            }
            else if (next_y < st->height)
            {
                player_y = next_y;
            }

            if ((player_y + 1 < st->height) && st->rows[player_y + 1][player_x] == '#')
            {
                is_jumping = 0;
                velocity_y = 0;
            }
        }
        else
        {
            if (floor_tile != '#' && floor_tile != 'H')
            {
                if (player_y + 1 < st->height) player_y++;
                else init_stage();
            }
        }
    }

    // 동적 높이 기준으로 맵 밖 추락 시 스테이지 리셋
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
            init_stage();
            return;
        }
    }
    for (int i = 0; i < coin_count; i++)
    {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y)
        {
            coins[i].collected = 1;
            score += 20;
        }
    }
}

// 비동기 키보드 입력 확인
int kbhit()
{
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
    // Black	30	40
    // Red	    31	41
    // Green	32	42
    // Yellow	33	43
    // Blue	    34	44
    // Magenta	35	45
    // Cyan	    36	46
    // White	37	47
    // Default	39	49

    // Bright Black	90	100
    // Bright Red	91	101
    // Bright Green	92	102
    // Bright Yellow93	103
    // Bright Blue	94	104
    // Bright Magenta   95	105
    // Bright Cyan	96	106
    // Bright White	97	107
}

//체력을 감소 시키고 게임 오버 여부 확인
void health_system()
{
    health--;
    if (health <= 0) //체력 소진 시 게임 오버
    {
        printf("Game Over\n");
        exit(0);
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
    printf("\x1b[2J\x1b[H"); // 화면 지우기
    usleep(200000);

    const char* frames[] = {
        " \n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n",

        " \n"
        "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
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
        "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\n"
        "        ( •_•)\t( •_•)\n"
        "        / >🍒\t/ >🍒\n",


        " \n"
        "     ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██║   ██║██████╔╝██║\n"
        "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/) \n"
        "        ( •_•)\t( •_•)\t( •_•) \n"
        "        / >🍒\t/ >🍒\t/ >🍒\n",

        " \n"
        "     ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔═══██╗██╔══██╗██║\n"
        "     ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██║   ██║██████╔╝██║\n"
        "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\n"
        "        ( •_•)\t( •_•)\t( •_•)\t( •_•)\n"
        "        / >🍒\t/ >🍒\t/ >🍒\t/ >🍒\n",

        " \n"
        "     ███╗   ██╗██╗   ██╗ ██████╗ ██╗   ██╗ ██████╗ ██████╗ ██╗\n"
        "     ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔═══██╗██╔══██╗██║\n"
        "     ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██║   ██║██████╔╝██║\n"
        "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\t(\\_/) \n"
        "        ( •_•)\t( •_•)\t( •_•)\t( •_•)\t( •_•) \n"
        "        / >🍒\t/ >🍒\t/ >🍒\t/ >🍒\t/ >🍒  \n",

        " \n"
        " \n"
        "     ███╗   ██╗██╗   ██╗ ██████╗ ██╗   ██╗ ██████╗ ██████╗ ██╗\n"
        "     ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔═══██╗██╔══██╗██║\n"
        "     ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██║   ██║██████╔╝██║\n"
        "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n"
        " \n"
        "        (\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\t(\\_/)\t(\\_/) \n"
        "        ( •_•)\t( •_•)\t( •_•)\t( •_•)\t( •_•)\t( •_•) \n"
        "        / >🍒\t/ >🍒\t/ >🍒\t/ >🍒\t/ >🍒\t/ >🍒  \n"

        //
        // " \n"
        // "     ███╗   ██╗██╗   ██╗ ██████╗ ██╗   ██╗ ██████╗ ██████╗ ██╗\n"
        // "     ████╗  ██║██║   ██║██╔════╝ ██║   ██║██╔═══██╗██╔══██╗██║\n"
        // "     ██╔██╗ ██║██║   ██║██║  ███╗██║   ██║██║   ██║██████╔╝██║\n"
        // "     ██║╚██╗██║██║   ██║██║   ██║██║   ██║██║   ██║██╔══██╗██║\n"
        // "     ██║ ╚████║╚██████╔╝╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║\n"
        // "     ╚═╝  ╚═══╝ ╚═════╝  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝\n",
    };

    int frame_count = sizeof(frames) / sizeof(frames[0]);

    for (int i = 0; i < frame_count; i++)
    {
        printf("\x1b[2J\x1b[H");
        printf("%s\n", frames[i]);
        usleep(500000);
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
        printf("\x1b[2J\x1b[H");  // 화면 클
        printf("%s\n", frames[count - 1]); // END
        printf("%s\n", frames[i]);    // 애니메이션 프레임
        printf("\n종료하려면 엔터...\n");
        usleep(500000);
        i = (i + 1) % (count-1);  // 프레임 순환
        // 엔터 키 입력 시 종료
        if (kbhit())
        {
            break;
        }
    }
}
