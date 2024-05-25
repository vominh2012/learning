#include <stdlib.h>
#include <time.h>
#include "sudoku.cpp"
#include "base.h"
#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define COLS 9
#define ROWS 9
#define BOARD_SIZE (COLS * ROWS)
#define PADDING 9
#define BOARD_MEM_SIZE (BOARD_SIZE  + PADDING)
#define BLOCK_SIZE 3
const int screenWidth = 900;
const int screenHeight = 900;

const int board_width = screenWidth / 3;
const int board_height = screenHeight / 3;
const int cellWidth = board_width / COLS;
const int cellHeight = board_height / ROWS;

struct SudokuStep {
    u32 step_id;
    
    u8 sudoku[BOARD_MEM_SIZE];
    u16 candidates[BOARD_MEM_SIZE * 3];
    
    u32 move_index;
    
    SudokuStep *next;
    SudokuStep *prev;
};

struct SudokuGame {
    u32 game_id;
    bool solved;
    
    u8 sudoku[BOARD_MEM_SIZE];
    u16 candidates[BOARD_MEM_SIZE * 3];
    
    u8 player_board[BOARD_MEM_SIZE];
    u16 player_board_candidates[BOARD_MEM_SIZE];
    int player_selected_row = 0;
    int player_selected_col = 0;
    
    u8 sudoku_solution[BOARD_MEM_SIZE];
    u32 num_guess;
    double time_solve;
    
    SudokuGame *next;
    SudokuGame *prev;
    
    SudokuStep debug_steps;
    SudokuStep *debug_current;
};

void sudoku_debug_callback(void *context, u8 *sudoku, u16 *sudoku_possible, u32 index, u32 val)
{
    SudokuGame *game = (SudokuGame *) context;
    
    SudokuStep * step = (SudokuStep*)malloc(sizeof(SudokuStep));
    
    memcpy(step->sudoku, sudoku, sizeof(step->sudoku));
    memcpy(step->candidates, sudoku_possible, sizeof(step->candidates));
    step->move_index = index;
    
    dll_insert_back(&game->debug_steps, step);
    step->step_id = step->prev->step_id + 1;
}

void sudoku_solve_from_file(char *fname, SudokuGame *sudoku_games)
{
    FILE *f = fopen(fname, "rb");
    if(f) {
        
        fseek(f, 0, SEEK_END);
        u32 file_size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        u8 *file_contents = (u8*)malloc(file_size + 1);
        file_contents[file_size] = 0;
        fread(file_contents, file_size, 1, f);
        fclose(f);
        
        u8 c = 0;
        
        const u32 max_row = 9;
        const u32 max_col = 9;
        const u32 padding = 32;
        const u32 max_size = 9 * 9;
        u32 solved_count = 0;
        
        u32 i = 0;
        
        // skip comment stuff
        while(i < file_size && file_contents[i] == '#')
        {
            while (i < file_size && file_contents[++i] != '\n');
            ++i;
        }
        
        u64 total_guess = 0;
        u32 game_id = 0;
        while (i < file_size)
        {
            SudokuGame *game = (SudokuGame*)calloc(sizeof(SudokuGame), 1);
            dll_init(&game->debug_steps);
            game->debug_current = &game->debug_steps;
            
            dll_insert_back(sudoku_games, game);
            game->game_id = game->prev->game_id + 1;
            u8 *sudoku = game->sudoku;
            u8 *sudoku_solution = game->sudoku_solution;
            u16* sudoku_possible = game->candidates;
            
            u8* buffer = file_contents + i;
            i += max_size;
            while (file_contents[i++] != '\n');
            
            for (int j = 0; j < max_size; ++j)
            {
                c = buffer[j];
                
                if (c == '.') c = '0';
                
                // 0 to 9 ascii
                if (c >= '0' && c <= (max_row + 48))
                {
                    sudoku[j] = c - 48;
                }
            }
            
            double time_start = GetTime();
            
            u32 bcount = 0;
            u32 num_guess = 0;
            u32 block_size = 3;
            
            memcpy(sudoku_solution, sudoku, max_size);
            memcpy(game->player_board, sudoku, max_size);
            
            sudoku_print(sudoku, max_size);
            
            sudoku_fill_possible(sudoku_solution, sudoku_possible, max_row, max_col, block_size);
            
            u16 *sudoku_candidates = (u16*)malloc(max_size * sizeof(u16) * 3);
            memcpy(sudoku_candidates, sudoku_possible, max_size * sizeof(u16) * 3);
            
            u16 *hidden_pair = sudoku_candidates + max_size;
            u16 *hidden_single = sudoku_candidates + max_size * 2;
            for (int i = 0; i < max_size; ++i)
            {
                if (hidden_pair[i]) sudoku_candidates[i] = hidden_pair[i];
                if (hidden_single[i]) sudoku_candidates[i] = hidden_single[i];
            }
            
            bool ret = sudoku_backtrack_heuristic(sudoku_solution, sudoku_candidates, max_row, max_col, block_size, &num_guess, sudoku_debug_callback, game);
            game->solved = ret;
            game->num_guess = num_guess;
            game->time_solve = GetTime() - time_start;
            //bool ret = sudoku_backtrack_min(sudoku_solution, max_row, max_col, block_size, &num_guess);
            
            total_guess += num_guess;
            sudoku_games->num_guess = total_guess;
            
            
            printf("Game ID: %d\n", game->game_id);
            printf("Number of guess: %d\n", num_guess);
            sudoku_print(sudoku_solution, max_size);
            fflush(stdout);
            
            if (!ret) {
                num_guess = 0;
                bool ret2 = sudoku_backtrack_min(sudoku_solution, max_row, max_col, block_size, &num_guess);
                
                printf("Number of guess: %d\n", num_guess);
                sudoku_print(sudoku_solution, max_size);
                fflush(stdout);
                
                if (ret2) {
                    printf("heurictic bug");
                    assert(ret);
                }
                else // no solution
                {
                    assert(ret);
                }
            }
            
            solved_count++;
            if (solved_count > 20) 
                break;
            
        }
        
        if (total_guess) {
            printf("Total guess/problem %lld/%d=%lld\n", total_guess, solved_count, total_guess/solved_count);
        }
        
        if (file_contents) free(file_contents);
    }
    
}

float timeGameStarted;
float timeGameEnded;

void CellDraw(Vector2 p, int r, int c, u8 val, Color default_color, u16 candidates, u16 hidden_single, u16 hidden_pair)
{
    Font font = GetFontDefault();
    
    float recx = p.x  + r * cellWidth;
    float recy = p.y + c * cellHeight;
    
    if (!candidates)
    {
        char s[2] = {0};
        s[0] = val + 48;
        
        float font_size = 20.0f;
        Vector2 text_size = MeasureTextEx(font, s, font_size, 0);
        DrawTextEx(font, s, {recx + cellWidth/2  - text_size.x/2, recy + cellHeight/2 - text_size.y/2}, font_size, 0, default_color);
    }
    else 
    {
        u16 val = 1;
        for (int i = 0; i < BLOCK_SIZE; ++i)
            for (int j = 0; j < BLOCK_SIZE; ++j)
        {
            {
                float sub_recx = recx + j * cellWidth / BLOCK_SIZE;
                float sub_recy = recy + i * cellHeight / BLOCK_SIZE;
                Color text_color = default_color;
                u16 bits = (1 << val);
                if (hidden_pair & bits)
                    text_color = BLUE;
                if (hidden_single & bits)
                    text_color = GREEN;
                
                if (candidates & bits)
                {
                    char s[2] = {0};
                    s[0] = val + 48;
                    
                    float font_size = 10.0f;
                    Vector2 text_size = MeasureTextEx(font, s, font_size, 0);
                    DrawTextEx(font, s, {sub_recx + cellWidth/BLOCK_SIZE/2 - text_size.x/2, sub_recy + cellHeight/BLOCK_SIZE/2 - text_size.y/2}, font_size, 0, text_color);
                }
                
                ++val;
            }
        }
    }
}

void draw_rect(Rectangle rec, float thick, Color c)
{
    DrawLineEx({rec.x , rec.y}, {rec.x + rec.width, rec.y}, thick, c);
    DrawLineEx({rec.x, rec.y}, {rec.x, rec.y + rec.height}, thick, c);
    DrawLineEx({rec.x + rec.width, rec.y}, {rec.x + rec.width, rec.y + rec.height}, thick, c);
    DrawLineEx({rec.x, rec.y + rec.height}, {rec.x + rec.width, rec.y + rec.height}, thick, c);
}

void draw_grid(Rectangle first_cell, int num_row, int num_col, float thick, Color c) {
    for (int i = 0; i < num_col; ++i)
    {
        for (int j = 0; j < num_row; ++j)
        {
            Vector2 a = {first_cell.x + first_cell.width  * i, first_cell.y + first_cell.height * j};
            Rectangle rec = {a.x, a.y, first_cell.width, first_cell.height};
            draw_rect(rec, thick, c);
        }
    }
}

void draw_board_grid(Vector2 pos)
{
    Rectangle cell_rec = {pos.x, pos.y, cellWidth, cellHeight};
    draw_grid(cell_rec, ROWS, COLS, 1, GRAY);
    
    Rectangle block_rec = {pos.x, pos.y, BLOCK_SIZE * cellWidth, BLOCK_SIZE *cellHeight};
    draw_grid(block_rec, BLOCK_SIZE, BLOCK_SIZE, 2, BLACK);
    
}

void draw_board(Vector2 pos,u8* sudoku, u16 *candidates = 0, u16 *hidden_pair = 0, u16* hidden_single = 0)
{
    draw_board_grid(pos);
    
    for (int i = 0; i < COLS; i++)
    {
        for (int j = 0; j < ROWS; j++)
        {
            int index = ROWS * j + i;
            u16 candidates_mask = candidates ? candidates[index] : 0;
            u16 hidden_single_mask = hidden_single ? hidden_single[index] : 0;
            u16 hidden_pair_mask = hidden_pair ? hidden_pair[index] : 0;
            CellDraw(pos, i, j, sudoku[index], DARKGRAY, candidates_mask, hidden_single_mask, hidden_pair_mask);
        }
    }
    
}

void draw_board_debug(Vector2 pos,u8* sudoku, u8 *solution = 0, u16 *candidates = 0, int move_index = 0, u16 *hidden_pair = 0, u16* hidden_single = 0)
{
    draw_board_grid(pos);
    
    for (int i = 0; i < COLS; i++)
    {
        for (int j = 0; j < ROWS; j++)
        {
            int index = ROWS * j + i;
            u16 candidates_mask = candidates ? candidates[index] : 0;
            u16 hidden_single_mask = hidden_single ? hidden_single[index] : 0;
            u16 hidden_pair_mask = hidden_pair ? hidden_pair[index] : 0;
            
            Color color = DARKGRAY;
            if (sudoku[index] && sudoku[index] != solution[index])
                color = RED;
            if (index == move_index)
            {
                float recx = pos.x  + i * cellWidth + 1;
                float recy = pos.y + j * cellHeight + 1;
                draw_rect({recx, recy, cellWidth - 3, cellHeight - 3}, 2, BLUE);
            }
            
            CellDraw(pos, i, j, sudoku[index], color, candidates_mask, hidden_single_mask, hidden_pair_mask);
        }
    }
    
}

void draw_player_board(Vector2 pos,u8* sudoku, u8 *player_sudoku, u16 *candidates = 0, u16 *hidden_pair = 0, u16* hidden_single = 0)
{
    draw_board_grid(pos);
    
    for (int i = 0; i < COLS; i++)
    {
        for (int j = 0; j < ROWS; j++)
        {
            int index = ROWS * j + i;
            u16 candidates_mask = candidates ? candidates[index] : 0;
            
            Color color = DARKGRAY;
            if (!sudoku[index])
                color = BLUE;
            
            if (player_sudoku[index])
                CellDraw(pos, i, j, player_sudoku[index], color, candidates_mask, 0, 0);
        }
    }
    
}

int main()
{
	srand(time(0));
    
    int padding = 10;
	InitWindow(screenWidth + padding, screenHeight + padding, "Sudoku Solver");
    
    timeGameStarted = GetTime();
	
    SudokuGame games = {};
    dll_init(&games);
    sudoku_solve_from_file("data/puzzles2_17_clue", &games);
    SudokuGame *game = games.next;
    
    timeGameEnded= GetTime();
    
    float x = 10.0f;
    float y = 30.0f;
    
    Vector2 player_board_pos = {x + board_width, y};
    Rectangle player_board_rec = {player_board_pos.x, player_board_pos.y , board_width, board_height};
    
    int debug_step_id = 0;
    
    while(!WindowShouldClose())
    {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            game->player_selected_row = game->player_selected_col = 0;
            
            Vector2 mPos = GetMousePosition();
            if (CheckCollisionPointRec(mPos, player_board_rec))
            {
                game->player_selected_col = (int)(mPos.x - player_board_rec.x) / cellWidth + 1;
                game->player_selected_row = (int)(mPos.y - player_board_rec.y) / cellHeight + 1;
            }
            
        }
        else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            Vector2 mPos = GetMousePosition();
            int indexI = (int)mPos.x / cellWidth;
            int indexJ = (int)mPos.y / cellHeight;
            
            
        }
        
        if (IsKeyPressed(KEY_N))
        {
            if (game->next != &games)
                game = game->next;
        }
        
        if (IsKeyPressed(KEY_P))
        {
            if (game->prev != &games)
                game = game->prev;
        }
        
        if (IsKeyPressed(KEY_J))
        {
            SudokuStep *step = &game->debug_steps;
            for (int i = 0; i < debug_step_id; ++i)
                step = step->next;
            
            game->debug_current = step;
            
        }
        
        
        if (IsKeyPressed(KEY_RIGHT))
        {
            if (game->debug_current->next != &game->debug_steps)
                game->debug_current = game->debug_current->next;
        }
        
        if (IsKeyPressed(KEY_LEFT))
        {
            if (game->debug_current->prev != &game->debug_steps)
                game->debug_current = game->debug_current->prev;
        }
        
        if (game->player_selected_row)
        {
            DrawText(TextFormat("cell selected: %d, %d", game->player_selected_row, game->player_selected_col), 200, 0, 10, DARKGRAY);
            
            int key_pressed = GetKeyPressed();
            static int last_key_pressed = 0;
            if (key_pressed) 
                last_key_pressed = key_pressed;
            if (last_key_pressed)
            {
                int number = last_key_pressed - '0';
                if (number > 0 && number < 10) {
                    int row = game->player_selected_row - 1;
                    int col = game->player_selected_col - 1;
                    int index = row * ROWS+ col;
                    if (!game->sudoku[index]) // only modify empty cell
                        game->player_board[index] = number;
                    last_key_pressed = 0;
                }
            }
        }
        
        DrawText(TextFormat("Game: %d, Success: %d, Guess: %d, Time solve: %f secs", game->game_id, game->solved, game->num_guess, game->time_solve), 5, 0, 10, DARKGRAY);
        
        DrawText(TextFormat("Total guess: %d, Total time: %f secs", games.num_guess, timeGameEnded - timeGameStarted), 5, 15, 10, DARKGRAY);
        
        // draw game
        
        // draw challenge
        u16 *candidates = game->candidates;
        u16 *hidden_pair = game->candidates + BOARD_SIZE;
        u16 *hidden_single = game->candidates + BOARD_SIZE * 2;
        draw_board({x, y}, game->sudoku, candidates, hidden_single, hidden_pair);
        
        // draw player board
        draw_player_board(player_board_pos, game->sudoku, game->player_board, game->player_board_candidates);
        
        
        // draw solution
        draw_board({player_board_pos.x + board_width, y}, game->sudoku_solution);
        
        // draw debug board
        if (game->debug_current)
        {
            float debug_x = x;
            float debug_y = y + board_height + 30;
            
            int value_box =  GuiValueBox({debug_x + 120, debug_y, 100, 20}, "debug_step_id", &debug_step_id, 1, -1, true);
            debug_y += 30;
            
            SudokuStep *step = game->debug_current;
            for(int i = 0; i < 3; ++i)
            {
                debug_x = x + i * board_width;
                DrawText(TextFormat("Step: %d", step->step_id), debug_x, debug_y, 10, DARKGRAY);
                if (step != &game->debug_steps)
                    draw_board_debug({debug_x, debug_y + 30}, step->sudoku, game->sudoku_solution, step->candidates, step->move_index);
                step = step->next;
            }
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    
    return 0;
}

