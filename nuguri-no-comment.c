#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
#endif
#include <time.h>

typedef struct
{
    int width;
    int height;
    char** rows;
} Stage;

typedef struct
{
    int x, y;
    int dir;
} Enemy;

typedef struct
{
    int x, y;
    int collected;
} Coin;

Stage* stages = NULL;
int stage_count = 0;
int player_x, player_y;
int spawn_x, spawn_y;
int stage = 0;
int score = 0;
const int MAX_HEALTH = 3;


int is_jumping = 0;
int velocity_y = 0;
int on_ladder = 0;
int health = 3;


Enemy* enemies = NULL;
int enemy_count = 0;
int enemy_capacity = 0;
Coin* coins = NULL;
int coin_count = 0;
int coin_capacity = 0;


int enemy_move_timer = 0;


char** display_rows = NULL;
char* display_buffer = NULL;
int display_width = 0;
int display_height = 0;


#ifndef _WIN32
struct termios orig_termios;
#endif


void disable_raw_mode();
void enable_raw_mode();

void append_stage(char** temp_lines, int temp_count, int max_width);
void load_maps();
void init_stage();
void draw_game();
void update_game(char input);
void move_player(char input);
void move_enemies();
void check_collisions();
void check_coin(int x, int y);
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
void show_cursor(void);
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

#ifndef _WIN32
    enable_raw_mode();
#endif
    atexit(show_cursor);
    atexit(cls_mem);
    load_maps();
    init_stage();

    int c = '\0';
    int game_over = 0;

    while (!game_over && stage < stage_count)
    {
        if (kbhit())
        {
            c = getch();
#ifdef _WIN32

            if (c == 0xE0)
            {
                c = getch();
                switch (c)
                {
                case 72: c = 'w';
                    break;
                case 80: c = 's';
                    break;
                case 77: c = 'd';
                    break;
                case 75: c = 'a';
                    break;
                }
            }
            else if (c == 'q')
            {
                game_over = 1;
                continue;
            }
#else
            if (c == 'q')
            {
                game_over = 1;
                continue;
            }
            if (c == '\x1b')
            {
                getch(); ['
                switch (getch())
                {
                case 'A': c = 'w';
                    break;
                case 'B': c = 's';
                    break;
                case 'C': c = 'd';
                    break;
                case 'D': c = 'a';
                    break;
                }
            }
#endif

        }
        else
        {
            c = '\0';
        }


        while (kbhit()) { getch(); }

        update_game(c);
        draw_game();
#ifdef _WIN32
        delay(30);
#else
        delay(90);
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


void append_stage(char** temp_lines, int temp_count, int max_width)
{
    if (temp_count == 0) return;


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


void load_maps()
{

    FILE* file = fopen("map.txt", "r");
    if (!file)
    {
        fprintf(stderr, "map.txt 파일을 열 수 없습니다\n");
        exit(1);
    }

    char** temp_lines = NULL;
    int temp_capacity = 0;
    int temp_count = 0;
    int max_width = 0;
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
            int new_capacity = temp_capacity ? temp_capacity * 2 : 32;
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


    free(enemies);
    enemies = NULL;



    free(coins);
    coins = NULL;



    free(display_buffer);
    display_buffer = NULL;



    free(display_rows);
    display_rows = NULL;
}



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
                spawn_x = x;
                spawn_y = y;
            }
            else if (cell == 'X')
            {


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


void draw_game()
{
    Stage* st = &stages[stage];
    cls_screen();
    printf("Stage: %d/%d | Score: %d\n", stage + 1, stage_count, score);
    printf("조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");
    draw_health();

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
                textcolor(8);
                break;
            case 'H':
                textcolor(6);
                break;
            case 'C':
                textcolor(3);
                break;
            case 'X':
                textcolor(1);
                break;
            case 'P':
                textcolor(2);
                break;
            default:
                textcolor(9);
                break;
            }
            printf("%c", cell);
            textcolor(9);
        }
        printf("\n");
    }
}


void update_game(char input)
{
    move_player(input);
    move_enemies();
    check_collisions();
}


void check_coin(int x, int y)
{
    for (int i = 0; i < coin_count; i++)
    {
        if (!coins[i].collected && coins[i].x == x && coins[i].y == y)
        {
            coins[i].collected = 1;
            score += 20;
            beep();
        }
    }
}


void move_player(char input)
{
    Stage* st = &stages[stage];
    char floor_tile = 0;
    char current_tile = 0;
    int next_x = player_x;


    switch (input)
    {
    case 'a': next_x--;
        break;
    case 'd': next_x++;
        break;
    }

    if (next_x >= 0 && next_x < st->width && st->rows[player_y][next_x] != '#')
    {
        player_x = next_x;
    }
    check_coin(player_x, player_y);


    floor_tile = (player_y + 1 < st->height) ? st->rows[player_y + 1][player_x] : '#';
    current_tile = st->rows[player_y][player_x];


    if (input == 's' && floor_tile == '#' && player_y + 2 < st->height && st->rows[player_y + 2][player_x] == 'H')
    {
        player_y += 2;
        check_coin(player_x, player_y);
    }



    else if (on_ladder && !is_jumping)
    {
        velocity_y = 0;
        if (input == 'w')
        {
            if (player_y - 1 >= 0 && st->rows[player_y - 1][player_x] != '#')
            {
                player_y--;
                check_coin(player_x, player_y);
            }
        }
        else if (input == 's')
        {
            if (player_y + 1 < st->height && st->rows[player_y + 1][player_x] != '#')
            {
                player_y++;
                check_coin(player_x, player_y);
            }
        }
    }


    on_ladder = (current_tile == 'H');


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


        if (!climbed && (floor_tile == '#' || on_ladder))
        {
            is_jumping = 1;
            velocity_y = -2;
        }
    }








    if (!is_jumping && (floor_tile == ' ' || floor_tile == 'C') && !on_ladder)
    {
        is_jumping = 1;
        velocity_y = 1;
    }

    if (is_jumping)
    {
        int steps = abs(velocity_y);
        int dir = (velocity_y > 0) ? 1 : -1;

        for (int i = 0; i < steps; i++)
        {
            int test_y = player_y + dir;


            if (test_y < 0 || test_y >= st->height)
            {
                if (test_y >= st->height) init_stage();
                velocity_y = 0;
                break;
            }

            char target_cell = st->rows[test_y][player_x];


            if (target_cell == '#')
            {
                velocity_y = 0;
                if (dir == 1)
                {
                    is_jumping = 0;
                }
                break;
            }


            player_y = test_y;
            check_coin(player_x, player_y);
        }


        if (is_jumping)
        {
            velocity_y++;
            if (velocity_y > 3) velocity_y = 3;
        }
    }


    if (player_y >= st->height) init_stage();
}


void move_enemies()
{
    enemy_move_timer++;
    if (enemy_move_timer < 3)
    {
        return;
    }
    else
    {
        enemy_move_timer = 0;
    }

    Stage* st = &stages[stage];
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


void check_collisions()
{
    for (int i = 0; i < enemy_count; i++)
    {
        if (player_x == enemies[i].x && player_y == enemies[i].y)
        {
            score = (score > 50) ? score - 50 : 0;
            health_system();
            player_x = spawn_x;
            player_y = spawn_y;
            is_jumping = 0;
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
    return _getch();
#else
    return getchar();
#endif
}

void textcolor(int color)
{

    if (color < 20)
    {
        printf("\033[%dm", color + 30);
    }
    else
    {
        printf("\033[%dm", color + 70);
    }
}


void health_system()
{
    health--;
    if (health <= 0)
    {
        void_screen();
        game_over();
    }
}


void draw_health()
{
    for (int i = 0; i < health; i++)
    {
        textcolor(1);
        printf("♥ ");
    }
    for (int i = 0; i < MAX_HEALTH - health; i++)
    {
        textcolor(1);
        printf("♡ ");
    }
    printf("\n");
    textcolor(9);
}

void opening(void)
{
    cls_screen();
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
        "                            \n"
        "                            \n"
        "        (\\_/)      (\\_/)    \n"
        "       ( ^_^ )    ( ^_^ )    \n"
        "        / >🍒     / >🍒       \n",

        "                            \n"
        "                            \n"
        "        (\\_/)      (\\_/)    \n"
        "       ( ^o^ )    ( ^o^ )    \n"
        "        / >🍒     / >🍒       \n",

        "                            \n"
        "        (\\_/)      (\\_/)    \n"
        "      \\( ^o^ )/  \\( ^o^ )/\n"
        "        /  🍒     /  🍒       \n"
        "                            \n",

        "                            \n"
        "                            \n"
        "        (\\_/)      (\\_/)     \n"
        "       ( ^o^ )    ( ^o^ )     \n"
        "        / >🍒     / >🍒        \n",

        "                            \n"
        "        (\\_/)      (\\_/)    \n"
        "      \\( ^o^ )/  \\( ^o^ )/\n"
        "        /  🍒     /  🍒       \n"
        "                            \n",

        "                            \n"
        "                            \n"
        "        (\\_/)      (\\_/)    \n"
        "       ( ^o^ )    ( ^o^ )    \n"
        "        / >🍒     / >🍒      \n",


        "                            \n"
        "                            \n"
        "        (\\_/)      (\\_/)    \n"
        "       ( -_- )    ( -_- )    \n"
        "       <  🍒\\     <  🍒\\    \n",

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


    int i = 0;
    void_screen();
    while (1)
    {
        cls_screen();
        printf("%s\n", frames[count - 1]);
        printf("%s\n", frames[i]);
        printf("\n종료하려면 아무키나 입력...\n");
        delay(500);
        i = (i + 1) % (count - 1);

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
    printf("\x1b[2J\x1b[H");
}


void cls_screen(void)
{
    printf("\x1b[H");
}


void hide_cursor(void)
{
    printf("\x1b[?25l");
}


void show_cursor(void)
{
    printf("\x1b[?25h");
}


void beep()
{
    printf("\a");
}


void delay(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
