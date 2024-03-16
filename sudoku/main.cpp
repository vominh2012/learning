#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#ifdef __builtin_log2
#define log2(x) __builtin_log2(x)
#else
// Fallback implementation (replace with a more efficient one if needed)
int log2(int x) {
    int count = 0;
    while (x > 1) {
        x >>= 1;
        count++;
    }
    return count;
}
#endif

void sudoku_print(u8 *sudoku, int max_size)
{
#if 0
    for (int k = 0; k < max_size; ++k)
    {
        if (k % 9 == 0)
            printf("\n");
        printf("%d ", sudoku[k]);
    }
    printf("\n");
#endif
}

bool read_sudoku_file(char *fname, u8 *sudoku, int max_row, int max_col, int max_size)
{
    FILE *f = fopen(fname, "rb");
    if(f) {
        u8 c = 0;
        int i = 0;
        while (fread(&c, 1, 1, f))
        {
            
            // skip solution info
            if (c == '=')
                while (fread(&c, 1, 1, f) && c != '\n');
            
            // 0 to 9 ascii
            if (c >= '0' && c <= (max_row + 48) && i < max_size)
            {
                sudoku[i++] = c - 48;
            }
        }
        
        fclose(f);
        
        return true;
    }
    
    return false;
}

int find_free(u8 *sudoku, int start_index, int max)
{
    for (int i = start_index; i < max; ++i)
    {
        if (sudoku[i] == 0)
            return i;
    }
    
    return max;
}

bool valid(u8 *sudoku, int val, int index, int max_row, int max_col, u8 block_size)
{
    // validate row
    int row = index / max_col;
    int start_row = row * max_col;
    int end_row = start_row +  max_col;
    for(int i = start_row; i < end_row; ++i)
    {
        if (sudoku[i] == val)
            return false;
    }
    
    // validate col
    int col = index % max_row;
    for(int i = 0; i < max_row; ++i)
    {
        int col_index = i * max_row + col;
        if (sudoku[col_index] == val)
            return false;
    }
    
    // validate block
    int start_block_col = col / block_size * block_size;
    int start_block_row = row / block_size * block_size;
    int start_block_index = start_block_row * max_col + start_block_col;
    for (int i = 0; i < block_size; ++i)
    {
        for(int j = 0; j < block_size; ++j)
        {
            const int block_index = start_block_index +  i * max_col + j;
            if (sudoku[block_index] == val)
                return false;
        }
    }
    
    
    return true;
}

bool sudoku_backtrack(u8 *sudoku_ret, int index, u8 max_row, u8 max_col, u8 block_size)
{
    int max_size = max_row * max_col;
    for(int i = index; i < max_size; ++i)
    {
        if (sudoku_ret[i] == 0)
        {
            for (int val = 1; val <= max_row; ++val)
            {
                if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) {
                    sudoku_ret[i] = val;
                    if (sudoku_backtrack(sudoku_ret, i + 1, max_row, max_col, block_size))
                    {
                        return true;
                    }
                    else {
                        sudoku_ret[i] = 0;
                    }
                }
            }
            
            return false;
        }
    }
    
    return true;
}

void sudoku_fill_possible(u8 *sudoku, u32* sudoku_possible, int max_row, int max_col, int block_size)
{
    int max_size = max_row * max_col;
    for(int i = 0; i < max_size; ++i)
    {
        if (sudoku[i] == 0) {
            for (int val = 1; val <= max_row; ++val) {
                if (valid(sudoku, val, i, max_row, max_col, block_size))
                {
                    // first 16 bit for marking possible number, second 16 bit for count possible
                    sudoku_possible[i] |= 1 << (15 + val);
                    sudoku_possible[i] += 1;
                }
            }
            
            u16 count = sudoku_possible[i] & 0xFFFF;
            if (count == 1) {
                u16 possible = sudoku_possible[i] >> 16;
                u8 val = (u16)log2(possible) + 1;
                sudoku[i] = val;
                sudoku_fill_possible(sudoku, sudoku_possible, max_row, max_col, block_size);
            }
            
        }
    }
}

int find_possible_min(u8 *sudoku, u32 max_row, u32 max_col, u32 block_size)
{
    u32 max_size = max_row * max_col;
    
    u32 min_index = max_size;
    u32 min_count = max_row;
    
    for(u32 i = 0; i < max_size; ++i)
    {
        if (sudoku[i] == 0) {
            
            u32 count = 0;
            for (u32 val = 1; val <= max_row; ++val) {
                if (valid(sudoku, val, i, max_row, max_col, block_size))
                {
                    //possible |= 1 << (val - 1); // tried this but doesn't help
                    count += 1;
                }
            }
            
            //sudoku_possible[i] = (possible << 16) |  count;
            if (count < min_count) {
                min_index = i;
                min_count = count;
            }
        }
    }
    
    return min_index;
}


bool sudoku_backtrack_min(u8 *sudoku_ret, u8 max_row, u8 max_col, u8 block_size)
{
    const u32 max_size = max_row * max_col;
    u32 i = find_possible_min(sudoku_ret, max_row, max_col, block_size);
    if (i < max_size)
    {
        for (u32 val = 1; val <= max_row; ++val)
        {
            if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) 
            {
                sudoku_ret[i] = val;
                if (sudoku_backtrack_min(sudoku_ret, max_row, max_col, block_size))
                {
                    return true;
                }
                else {
                    sudoku_ret[i] = 0;
                }
            }
        }
        
        return false;
    }
    
    return true;
}

bool sudoku_backtrack_with_possible(u8 *sudoku_ret, u32 *sudoku_possible, int index, u8 max_row, u8 max_col, u8 block_size)
{
    int max_size = max_row * max_col;
    for(int i = index; i < max_size; ++i)
    {
        if (sudoku_ret[i] == 0)
        {
            u32 possible_data = (sudoku_possible[i] >> 16);
            for (u32 val = 1; val <= max_row; ++val)
            {
                u32 possible = (1 << (val - 1)) & possible_data;
                if (possible)
                {
                    if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) {
                        sudoku_ret[i] = val;
                        if (sudoku_backtrack_with_possible(sudoku_ret, sudoku_possible, i + 1, max_row, max_col, block_size))
                        {
                            return true;
                        }
                        else {
                            sudoku_ret[i] = 0;
                        }
                    }
                }
            }
            
            return false;
        }
    }
    
    return true;
}

void test(char *fproblem, char *fsolution)
{
    const u8 max_row = 9;
    const u8 max_col = 9;
    const u8 block_size = 3;
    
    const int max_size = max_row * max_col;
    u32 sudoku_possible[max_size] = {};
    u8 sudoku[max_size] = {};
    u8 sudoku_s[max_size] = {};
    
    assert(read_sudoku_file(fproblem, sudoku, max_row, max_col, max_size));
    assert(read_sudoku_file(fsolution, sudoku_s, max_row, max_col, max_size));
    sudoku_print(sudoku, max_size);
    sudoku_print(sudoku_s, max_size);
    
#if 1
    sudoku_backtrack_min(sudoku, max_row, max_col, block_size);
#endif 
    
#if 0
    sudoku_fill_possible(sudoku, sudoku_possible, max_row, max_col, block_size);
    int index = find_free(sudoku, 0, max_size);
    sudoku_backtrack_with_possible(sudoku,  sudoku_possible, index, max_row, max_col, block_size);
#endif 
    
#if 0
    int index = find_free(sudoku, 0, max_size);
    sudoku_backtrack(sudoku, index, max_row, max_col, block_size);
#endif
    
    sudoku_print(sudoku, max_size);
    int ret = memcmp(sudoku, sudoku_s, max_size);
    assert(ret == 0);
    
}

void run_tests()
{
    test("sudokus/s01a.txt", "solutions/s01a_s.txt");
    test("sudokus/s01b.txt", "solutions/s01b_s.txt");
    test("sudokus/s01c.txt", "solutions/s01c_s.txt");
    test("sudokus/s02a.txt", "solutions/s02a_s.txt");
    test("sudokus/s02b.txt", "solutions/s02b_s.txt");
    test("sudokus/s02c.txt", "solutions/s02c_s.txt");
    test("sudokus/s03a.txt", "solutions/s03a_s.txt");
    test("sudokus/s03b.txt", "solutions/s03b_s.txt");
    test("sudokus/s03c.txt", "solutions/s03c_s.txt");
    test("sudokus/s04a.txt", "solutions/s04a_s.txt");
    test("sudokus/s04b.txt", "solutions/s04b_s.txt");
    test("sudokus/s04c.txt", "solutions/s04c_s.txt");
    test("sudokus/s05a.txt", "solutions/s05a_s.txt");
    test("sudokus/s05b.txt", "solutions/s05b_s.txt");
    test("sudokus/s05c.txt", "solutions/s05c_s.txt");
    test("sudokus/s06a.txt", "solutions/s06a_s.txt");
    test("sudokus/s06b.txt", "solutions/s06b_s.txt");
    test("sudokus/s06c.txt", "solutions/s06c_s.txt");
    test("sudokus/s07a.txt", "solutions/s07a_s.txt");
    test("sudokus/s07b.txt", "solutions/s07b_s.txt");
    test("sudokus/s07c.txt", "solutions/s07c_s.txt");
    test("sudokus/s08a.txt", "solutions/s08a_s.txt");
    test("sudokus/s08b.txt", "solutions/s08b_s.txt");
    test("sudokus/s08c.txt", "solutions/s08c_s.txt");
    test("sudokus/s09a.txt", "solutions/s09a_s.txt");
    test("sudokus/s09b.txt", "solutions/s09b_s.txt");
    test("sudokus/s09c.txt", "solutions/s09c_s.txt");
    test("sudokus/s10a.txt", "solutions/s10a_s.txt");
    test("sudokus/s10b.txt", "solutions/s10b_s.txt");
    test("sudokus/s10c.txt", "solutions/s10c_s.txt");
    test("sudokus/s11a.txt", "solutions/s11a_s.txt");
    test("sudokus/s11b.txt", "solutions/s11b_s.txt");
    test("sudokus/s11c.txt", "solutions/s11c_s.txt");
    test("sudokus/s12a.txt", "solutions/s12a_s.txt");
    test("sudokus/s12b.txt", "solutions/s12b_s.txt");
    test("sudokus/s12c.txt", "solutions/s12c_s.txt");
    test("sudokus/s13a.txt", "solutions/s13a_s.txt");
    test("sudokus/s13b.txt", "solutions/s13b_s.txt");
    test("sudokus/s13c.txt", "solutions/s13c_s.txt");
    test("sudokus/s14a.txt", "solutions/s14a_s.txt");
    test("sudokus/s14b.txt", "solutions/s14b_s.txt");
    test("sudokus/s14c.txt", "solutions/s14c_s.txt");
    test("sudokus/s15a.txt", "solutions/s15a_s.txt");
    test("sudokus/s15b.txt", "solutions/s15b_s.txt");
    test("sudokus/s15c.txt", "solutions/s15c_s.txt");
    test("sudokus/s16.txt", "solutions/s16_s.txt");
}
int main()
{
    for (int i = 0; i < 10; ++i)
        run_tests();
}

