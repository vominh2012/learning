#include <stdlib.h>
#include <time.h>
#include "sudoku.cpp"
#include "common.h"
#include "raylib.h"
#include "raymath.h"

#define COLS 9
#define ROWS 9
#define BOARD_SIZE (COLS * ROWS)
#define PADDING 9
#define BOARD_MEM_SIZE (BOARD_SIZE  + PADDING)
#define BLOCK_SIZE 3
const int screenWidth = 800;
const int screenHeight = 800;

const int cellWidth = screenWidth / COLS/2;
const int cellHeight = screenHeight / ROWS/2;

struct SudokuGame {
    u8 sudoku[BOARD_MEM_SIZE];
    u16 candidates[BOARD_MEM_SIZE * 3];
    u8 sudoku_solution[BOARD_MEM_SIZE];
    u32 num_guess;
    
    SudokuGame *next;
    SudokuGame *prev;
};

void sudoku_solve_from_file(char *fname, SudokuGame *sudoku_games)
{
    FILE *f = fopen(fname, "rb");
    if(f) {
        dll_init(sudoku_games);
        
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
        while (i < file_size)
        {
            SudokuGame *game = (SudokuGame*)calloc(sizeof(SudokuGame), 1);
            dll_insert_back(sudoku_games, game);
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
            
            u32 bcount = 0;
            //bool ret = sudoku_backtrack(sudoku, 0, 9, 9, 3, &bcount);
            //advance_sudoku(sudoku, 9, max_size, padding);
            u32 num_guess = 0;
            u32 block_size = 3;
            
            
            memcpy(sudoku_solution, sudoku, max_size);
            
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
            
            bool ret = sudoku_backtrack_heuristic(sudoku_solution, sudoku_candidates, max_row, max_col, block_size, &num_guess);
            game->num_guess = num_guess;
            
            //bool ret = sudoku_backtrack_min(sudoku_solution, max_row, max_col, block_size, &num_guess);
            
            total_guess += num_guess;
            
            
            printf("Number of guess: %d\n", num_guess);
            sudoku_print(sudoku_solution, max_size);
            fflush(stdout);
            assert(ret == true);
            
            solved_count++;
            if (solved_count > 5) 
                break;
            
            //memset(sudoku_possible, 0, max_size * sizeof(u16));
            //memset(sudoku, 0, max_size);
            assert(ret);
        }
        
        if (total_guess) {
            printf("Total guess/problem %lld/%d=%lld\n", total_guess, solved_count, total_guess/solved_count);
        }
        
        if (file_contents) free(file_contents);
    }
    
}

typedef enum GameState
{
	PLAYING,
	LOSE,
	WIN
} GameState;

GameState state;

float timeGameStarted;
float timeGameEnded;


void CellDraw(Vector2 p, int r, int c, u8 val, u16 candidates, u16 hidden_single, u16 hidden_pair)
{
    Font font = GetFontDefault();
    int x = (int)p.x;
    int y = (int)p.y;
    
    int recx = x  + r * cellWidth;
    int recy = y + c * cellHeight;
    DrawRectangleLines(recx, recy, cellWidth, cellHeight, BLACK);
    
    if (!candidates)
    {
        char s[2] = {0};
        s[0] = val + 48;
        
        float font_size = 20.0f;
        Vector2 text_size = MeasureTextEx(font, s, font_size, 0);
        DrawTextEx(font, s, {recx + cellWidth/2  - text_size.x/2, recy + cellHeight/2 - text_size.y/2}, font_size, 0, DARKGRAY);
    }
    else 
    {
        u16 val = 1;
        for (int i = 0; i < BLOCK_SIZE; ++i)
            for (int j = 0; j < BLOCK_SIZE; ++j)
        {
            {
                int sub_recx = recx + j * cellWidth / BLOCK_SIZE;
                int sub_recy = recy + i * cellHeight / BLOCK_SIZE;
                //4DrawRectangleLines(sub_recx, sub_recy, cellWidth / BLOCK_SIZE, cellHeight / BLOCK_SIZE, BLACK);
                Color text_color = DARKGRAY;
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

int main()
{
	srand(time(0));
    
	InitWindow(screenWidth, screenHeight, "Raylib Sudoku");
    
    state = PLAYING;
    timeGameStarted = GetTime();
	
    SudokuGame games = {};
    sudoku_solve_from_file("data/puzzles2_17_clue", &games);
    SudokuGame *game = games.next;
    
    timeGameEnded= GetTime();
    
	while(!WindowShouldClose())
	{
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			Vector2 mPos = GetMousePosition();
			int indexI = (int)mPos.x / cellWidth;
			int indexJ = (int)mPos.y / cellHeight;
            
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
        
		BeginDrawing();
        
        ClearBackground(RAYWHITE);
        DrawText(TextFormat("Time solve: %f secs", timeGameEnded - timeGameStarted), 5, 0, 10, DARKGRAY);
        
        DrawText(TextFormat("Num guess: %d", game->num_guess), 5, 15, 10, DARKGRAY);
        
        float x = screenWidth / 2 - cellWidth * COLS / 2;
        Vector2 board1_pos = {x, 0.0};
        for (int i = 0; i < COLS; i++)
        {
            for (int j = 0; j < ROWS; j++)
            {
                int index = ROWS * j + i;
                u16 *candidates = game->candidates;
                u16 *hidden_pair = game->candidates + BOARD_SIZE;
                u16 *hidden_single = game->candidates + BOARD_SIZE * 2;
                CellDraw(board1_pos, i, j, game->sudoku[index], candidates[index], hidden_single[index], hidden_pair[index]);
            }
        }
        
        Vector2 board2_pos = {x, screenHeight / 2};
        for (int i = 0; i < COLS; i++)
        {
            for (int j = 0; j < ROWS; j++)
            {
                int index = ROWS * j + i;
                CellDraw(board2_pos, i, j, game->sudoku_solution[index], 0, 0, 0);
            }
        }
        
		EndDrawing();
	}
	
	CloseWindow();
	
	return 0;
}

