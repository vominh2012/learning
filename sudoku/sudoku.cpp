#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include "nmmintrin.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#ifdef __builtin_log2
#define log2(x) __builtin_log2(x)
#else
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
#if 1
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
            
            // skip solution info & comment stuff
            if (c == '=' || c == '#')
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

void write_sudoku_file(char *fname, u8 *sudoku, int max_size, int max_row)
{
    FILE *f = fopen(fname, "wb");
    if (f) {
        int i = 0;
        while(i < max_size) {
            fprintf(f, "%d, ", sudoku[i]);
            ++i;
            if (i % max_row == 0) {
                fwrite("\n", 1, 1, f);
            }
        }
        
        fclose(f);
    }
}


u32 find_free(u8 *sudoku, u32 start_index, u32 max)
{
    for (u32 i = start_index; i < max; ++i)
    {
        if (sudoku[i] == 0)
            return i;
    }
    
    return max;
}


inline bool sse_is_valid_9(u8 *row, u8 val) 
{
    // loading 16 bytes so we need padding with row
    __m128i row_bits = _mm_loadu_si128((__m128i*)(row));
    __m128i mask = _mm_set1_epi8 (val);
    __m128i ret = _mm_cmpeq_epi8(row_bits, mask);
    int count = _mm_movemask_epi8(ret) & 0x1FF; // only get 9 bytes result
    return count == 0;
}

bool valid(u8 *sudoku, u8 val, int index, int max_row, int max_col, u8 block_size)
{
    // validate row
    int row = index / max_col;
    int start_row = row * max_col;
    int end_row = start_row +  max_col;
    
#if 1
    // using sse on row do improve perf but not col
    bool is_valid_row = sse_is_valid_9(sudoku + start_row, val); 
    if (!is_valid_row) return false;
#else
    for(int i = start_row; i < end_row; ++i)
    {
        if (sudoku[i] == val)
            return false;
    }
#endif
    
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

bool sudoku_backtrack(u8 *sudoku_ret, u32 index, u8 max_row, u8 max_col, u8 block_size, u32 *count)
{
    
    ++*count;
    if (*count > 387420489) {
        return false;
    }
    
    u32 max_size = max_row * max_col;
    
    for(u32 i = index; i < max_size; ++i)
    {
        if (sudoku_ret[i] == 0)
        {
            for (u8 val = 1; val <= max_row; ++val)
            {
                if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) {
                    sudoku_ret[i] = val;
                    if (sudoku_backtrack(sudoku_ret, i + 1, max_row, max_col, block_size, count))
                    {
                        return true;
                    }
                    
                    sudoku_ret[i] = 0;
                }
            }
            
            return false;
        }
    }
    
    return true;
}

bool sudoku_backtrack_r(u8 *sudoku_ret, u32 index, u8 max_row, u8 max_col, u8 block_size, u32 *count)
{
    
    ++*count;
    if (*count > 387420489) {
        return false;
    }
    
    u32 max_size = max_row * max_col;
    
    for(u32 i = index; i < max_size; ++i)
    {
        if (sudoku_ret[i] == 0)
        {
            for (u8 val = max_row; val > 0; --val)
            {
                if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) {
                    sudoku_ret[i] = val;
                    if (sudoku_backtrack_r(sudoku_ret, i + 1, max_row, max_col, block_size, count))
                    {
                        return true;
                    }
                    
                    sudoku_ret[i] = 0;
                }
            }
            
            return false;
        }
    }
    
    return true;
}

u16 count_set_bits(u16 n) {
    u32 count = 0;
    while (n) {
        n &= (n - 1); // Removes the rightmost set bit in each iteration
        count++;
    }
    return count;
}

void print_candidates(u16 candidates) 
{
    for (u8 val = 1; val <= 9; ++val) {
        if (1 << val & candidates)
        {
            printf("%d ", val);
        }
    }
    printf("\n");
}

bool sudoku_reduce_possible(u8 *sudoku, u16* sudoku_possible, int index, int val, int max_row, int max_col, int block_size)
{
    int max_size = max_row * max_col;
    
    // validate row
    int row = index / max_col;
    int start_row = row * max_col;
    int end_row = start_row +  max_col;
    
    sudoku_possible[index] = 0;
    
    u16 possible = 1 << val;
    
    
    for(int i = start_row; i < end_row; ++i)
    {
        if (sudoku_possible[i] & possible) {
            sudoku_possible[i] ^= possible;
            
            if (!sudoku_possible[i]) // zero possible mean we pick wrong number
                return false;
        }
    }
    
    // validate col
    int col = index % max_row;
    for(int i = 0; i < max_row; ++i)
    {
        int col_index = i * max_row + col;
        if (sudoku_possible[col_index] & possible) {
            sudoku_possible[col_index] ^= possible;
            
            if (!sudoku_possible[col_index]) // zero possible mean we pick wrong number
                return false;
        }
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
            if (sudoku_possible[block_index] & possible) {
                sudoku_possible[block_index] ^= possible;
                
                if (!sudoku_possible[block_index]) // zero possible mean we pick wrong number
                    return false;
            }
        }
    }
    
    
    
    return true;
}

void sudoku_fill_possible(u8 *sudoku, u16* sudoku_possible, int max_row, int max_col, int block_size)
{
    int max_size = max_row * max_col;
    for(int i = 0; i < max_size; ++i)
    {
        if (sudoku[i] == 0) {
            for (u16 val = 1; val <= max_row; ++val) {
                if (valid(sudoku, val, i, max_row, max_col, block_size))
                {
                    // first 16 bit for marking possible number, second 16 bit for count possible
                    sudoku_possible[i] |= 1 << val;
                    //sudoku_possible[i] += 1;
                }
            }
        }
    }
    
    // hidden naked
    u16 *hidden_pair = sudoku_possible + max_size;
    u16 *hidden_single = sudoku_possible + max_size * 2;
    
    int delta = 0;
    
#if 0
    // row
    for (int row = 0; row < max_row; ++row)
    {
        int start_row = row * max_row;
        int end_row = start_row + max_row;
        for(int i = start_row; i < end_row; ++i)
        {
            if (hidden_pair[i]) continue;
            
            for(int j = i + 1; j < end_row; ++j)
            {
                if (hidden_pair[j]) continue;
                
                u16 bits = sudoku_possible[i] & sudoku_possible[j];
                if (count_set_bits(bits) == 2) {
                    u16 row_bits = 0;
                    for (int k = start_row; k < end_row; ++k)
                    {
                        if (k != i && k != j) {
                            row_bits |= sudoku_possible[k];
                        }
                    }
                    
                    if ((bits & row_bits) == 0)
                    {
                        printf("found naked hidden at: %d, %d and %d, %d\n", row, i, row, j);
                        print_candidates(sudoku_possible[i]);
                        print_candidates(sudoku_possible[j]);
                        print_candidates(bits);
                        hidden_pair[i] = bits;
                        hidden_pair[j] = bits;
                    }
                }
            }
        }
    }
    
    // col
    for (int col = 0; col < max_col; ++col) 
    {
        for(int i = 0; i < max_row; ++i)
        {
            for(int j = i; j < max_row; ++j)
            {
                int col_i = i * max_row + col;
                int col_j = j * max_row + col;
                if (hidden_pair[col_i] || hidden_pair[col_j]) continue;
                
                u16 bits = sudoku_possible[col_i] & sudoku_possible[col_j];
                if (count_set_bits(bits) == 2) {
                    u16 col_bits = 0;
                    for (int k = 0; k < max_row; ++k) {
                        int col_k = col * max_row + k;
                        if (col_k != col_i && col_k != col_j)
                        {
                            col_bits |= sudoku_possible[col_k];
                        }
                    }
                    
                    if ((bits & col_bits) == 0)
                    {
                        printf("found naked hidden at: %d, %d and %d, %d\n", i, col, j, col);
                        print_candidates(sudoku_possible[col_i]);
                        print_candidates(sudoku_possible[col_j]);
                        print_candidates(bits);
                        hidden_pair[col_i] = bits;
                        hidden_pair[col_j] = bits;
                    }
                }
            }
        }
    }
    
    // block
    delta = 0;
    for (int b = 0; b < 9; ++b)
    {
        int start_block_index = b * block_size + delta * max_col;
        if ((b + 1)% 3 == 0) delta += 2;
        
        int box_indexs[9] = {0};
        int bi = 0;
        for (int i = 0; i < block_size; ++i)
        {
            for(int j = 0; j < block_size; ++j)
            {
                const int block_index = start_block_index +  i * max_col + j;
                box_indexs[bi++] = block_index;
            }
        }
        
        for (int i = 0; i < bi; ++i) {
            for (int j = i; j < bi; ++j)
            {
                int xi = box_indexs[i];
                int xj = box_indexs[j];
                u16 bits = sudoku_possible[xi] & sudoku_possible[xj];
                if (count_set_bits(bits) == 2) {
                    u16 row_bits = 0;
                    for (int k = 0; k < bi; ++k)
                    {
                        int xk = box_indexs[k];
                        if (k != i && k != j) {
                            row_bits |= sudoku_possible[xk];
                        }
                    }
                    
                    if ((bits & row_bits) == 0)
                    {
                        printf("box found naked hidden at: %d and %d\n", xi, xj);
                        print_candidates(sudoku_possible[xi]);
                        print_candidates(sudoku_possible[xj]);
                        print_candidates(bits);
                        hidden_pair[xi] = bits;
                        hidden_pair[xj] = bits;
                    }
                }
            }
        }
        
    }
    
#endif
    
#if 0
    // row
    for (int row = 0; row < max_row; ++row)
    {
        int start_row = row * max_row;
        int end_row = start_row + max_row;
        
        for (u32 val = 1; val <= max_row; ++val)
        {
            u16 val_bit = (1 << val);
            u32 count = 0;
            u32 found_index = 0;
            for(int i = start_row; i < end_row; ++i)
            {
                if (sudoku_possible[i] & val_bit) {
                    ++count;
                    found_index = i;
                }
            }
            
            if (count == 1) {
                hidden_single[found_index] |= val_bit;
                printf("row found hidden single at: %d val %d\n", found_index, val);
            }
        }
    }
    
    // col
    for (int col = 0; col < max_col; ++col) 
    {
        for (u32 val = 1; val <= max_row; ++val)
        {
            u16 val_bit = (1 << val);
            u32 count = 0;
            u32 found_index = 0;
            for(int i = 0; i < max_row; ++i)
            {
                int col_i = i * max_row + col;
                if (sudoku_possible[col_i] & val_bit) {
                    ++count;
                    found_index = col_i;
                }
            }
            
            if (count == 1) {
                hidden_single[found_index] |= val_bit;
                print_candidates(sudoku_possible[found_index]);
                printf("col found hidden single at %d val %d\n", found_index, val);
            }
        }
    }
    
    // block
    delta = 0;
    for (int b = 0; b < 9; ++b)
    {
        int start_block_index = b * block_size + delta * max_col;
        if ((b + 1)% 3 == 0) delta += 2;
        
        int box_indexs[9] = {0};
        int bi = 0;
        for (int i = 0; i < block_size; ++i)
        {
            for(int j = 0; j < block_size; ++j)
            {
                const int block_index = start_block_index +  i * max_col + j;
                box_indexs[bi++] = block_index;
            }
        }
        
        // loop through indexs
        for (u32 val = 1; val <= max_row; ++val)
        {
            u16 val_bit = (1 << val);
            u32 count = 0;
            u32 found_index = 0;
            for (int i = 0; i < bi; ++i) {
                int ki = box_indexs[i];
                if (sudoku_possible[ki] & val_bit) {
                    ++count;
                    found_index = ki;
                }
            }
            
            if (count == 1) {
                hidden_single[found_index] |= val_bit;
                print_candidates(sudoku_possible[found_index]);
                printf("box found hidden single at %d val %d\n", found_index, val);
            }
        }
    }
#endif
}

void sudoku_update_possible(u8 *sudoku, u16* sudoku_possible, int max_row, int max_col, int block_size)
{
    int max_size = max_row * max_col;
    for(int i = 0; i < max_size; ++i)
    {
        if (sudoku[i] == 0) {
            u16 possible = 0;
            for (u8 val = 1; val <= max_row; ++val) {
                if (valid(sudoku, val, i, max_row, max_col, block_size))
                {
                    // first 16 bit for marking possible number, second 16 bit for count possible
                    possible |= 1 << val;
                }
            }
            
            sudoku_possible[i] = possible;
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
            for (u8 val = 1; val <= max_row; ++val) {
                if (valid(sudoku, val, i, max_row, max_col, block_size))
                {
                    count += 1;
                }
            }
            
            if (count < min_count) {
                min_index = i;
                min_count = count;
            }
        }
    }
    
    return min_index;
}


bool sudoku_backtrack_min(u8 *sudoku_ret, u8 max_row, u8 max_col, u8 block_size, u32 *num_guess = 0)
{
    const u32 max_size = max_row * max_col;
    u32 i = find_possible_min(sudoku_ret, max_row, max_col, block_size);
    if (i < max_size)
    {
        for (u8 val = 1; val <= max_row; ++val)
        {
            
            if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) 
            {
                if (num_guess)
                    ++*num_guess;
                sudoku_ret[i] = val;
                
                if (sudoku_backtrack_min(sudoku_ret, max_row, max_col, block_size, num_guess))
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

bool sudoku_backtrack_with_possible(u8 *sudoku_ret, u16 *sudoku_possible, int index, u8 max_row, u8 max_col, u8 block_size, u32 *num_guess = 0)
{
    int max_size = max_row * max_col;
    for(int i = index; i < max_size; ++i)
    {
        if (sudoku_ret[i] == 0)
        {
            u16 possible_data = sudoku_possible[i];
            for (u8 val = 1; val <= max_row; ++val)
            {
                u16 possible = (1 << val) & possible_data;
                if (possible)
                {
                    if (num_guess) ++*num_guess;
                    
                    if (valid(sudoku_ret, val, i, max_row, max_col, block_size)) {
                        sudoku_ret[i] = val;
                        if (sudoku_backtrack_with_possible(sudoku_ret, sudoku_possible, i + 1, max_row, max_col, block_size, num_guess))
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

int find_possible_min_from_table(u8 *sudoku, u16 *possible, u32 max_size)
{
    
    u32 min_index = max_size;
    u32 min_count = max_size;
    
    for(u32 i = 0; i < max_size; ++i)
    {
        if (sudoku[i] == 0) {
            u32 count = count_set_bits(possible[i]);
            if (count < min_count) {
                min_index = i;
                min_count = count;
            }
        }
    }
    
    return min_index;
}

typedef void (*debug_callback)(void *context, u8 *sudoku, u16 *sudoku_possible, u32 index, u32 val);

bool sudoku_backtrack_heuristic(u8 *sudoku, u16 *sudoku_possible, u8 max_row, u8 max_col, u8 block_size, u32 *num_guess, debug_callback db_callback = 0, void *debug_context = 0)
{
    u32 max_size = max_row * max_col;
    
    u32 i = find_possible_min_from_table(sudoku, sudoku_possible, max_size);
    if (i < max_size)
    {
        u16 possible_data = sudoku_possible[i];
        for (u8 val = 1; val <= max_row; ++val)
        {
            u16 possible = (1 << val) & possible_data;
            if (possible)
            {
                
                if (valid(sudoku, val, i, max_row, max_col, block_size)) {
                    if (num_guess) ++*num_guess;
                    sudoku[i] = val;
                    
                    
                    u32 size = sizeof(u16) * max_size * 3;
                    u16 *new_possible = (u16*)malloc(size);
                    memcpy(new_possible, sudoku_possible, size); 
                    bool valid_pick = (sudoku_reduce_possible(sudoku, new_possible, i, val, max_row, max_col, block_size));
                    
                    if (db_callback) {
                        db_callback(debug_context, sudoku, new_possible, i, val);
                    }
                    if (valid_pick && sudoku_backtrack_heuristic(sudoku, new_possible, max_row, max_col, block_size, num_guess, db_callback, debug_context))
                    {
                        free(new_possible);
                        return true;
                    }
                    else {
                        free(new_possible);
                        sudoku[i] = 0;
                    }
                }
            }
        }
        
        return false;
    }
    
    return true;
}

void test(char *fproblem, char *fsolution, bool print_result, int algorithm = 0)
{
    const u8 max_row = 9;
    const u8 max_col = 9;
    const u8 block_size = 3;
    
    const int max_size = max_row * max_col;
    u16 sudoku_possible[max_size] = {};
    u8 sudoku[max_size] = {};
    u8 sudoku_s[max_size] = {};
    
    assert(read_sudoku_file(fproblem, sudoku, max_row, max_col, max_size));
    assert(read_sudoku_file(fsolution, sudoku_s, max_row, max_col, max_size));
    
    if (print_result)
    {
        sudoku_print(sudoku, max_size);
        sudoku_print(sudoku_s, max_size);
    }
    
    if (algorithm == 0)
    {
        sudoku_backtrack_min(sudoku, max_row, max_col, block_size);
    }
    else if (algorithm == 1)
    {
        sudoku_fill_possible(sudoku, sudoku_possible, max_row, max_col, block_size);
        int index = find_free(sudoku, 0, max_size);
        sudoku_backtrack_with_possible(sudoku,  sudoku_possible, index, max_row, max_col, block_size);
    }
    else if (algorithm == 2)
    {
        
        u32 count = 0;
        sudoku_backtrack(sudoku, 0, max_row, max_col, block_size, &count);
    }
    
    if (print_result)
    {
        sudoku_print(sudoku, max_size);
    }
    int ret = memcmp(sudoku, sudoku_s, max_size);
    assert(ret == 0);
    
}

void generate_sudoku(u32 level, u32 max_row, u32 max_col, u32 block_size) 
{
    srand((u32)time(NULL));
    
    u8 sudoku[9 * 9] = {};
    
    const u32 max_size = max_col * max_row;
    assert(max_size == 9 * 9);
    assert(block_size == 3);
    
    u32 count = 0;
    u32 backtrack_try_times = 0;
    while(true) {
        u32 i = 0;
        do {
            i = rand() % max_size;
            assert(i < max_size); 
        } while (sudoku[i] != 0); 
        
        int val = 0;
        int try_times = 0;
        do {
            if (try_times > 100) { val = 0; break; }
            val = rand() % max_row + 1;
        }while (!valid(sudoku, val, i, max_row, max_col, block_size));
        
        if (backtrack_try_times > 100) {
            memset(sudoku, 0, max_size);
            count = 0;
            backtrack_try_times = 0;
            
            printf("reset\n");
            
            continue;
        }
        
        if (val > 0) {
            assert(valid(sudoku, val, i, max_row, max_col, block_size));
            
            u8 sudoku_temp[9 * 9]  = {};
            memcpy(sudoku_temp, sudoku, max_size);
            sudoku_temp[i] = val;
            
            u32 bcount = 0;
            if (sudoku_backtrack(sudoku_temp, 0, max_row, max_col, block_size, &bcount)) {
                sudoku[i] = val;
                ++count;
                if (count >= level * max_size / 100) {
                    
                    u8 sudoku_temp1[9 * 9];
                    memcpy(sudoku_temp1, sudoku, max_size);
                    bcount = 0;
                    sudoku_backtrack_r(sudoku_temp1, 0, max_row, max_col, block_size, &bcount);
                    int ret = memcmp(sudoku_temp, sudoku_temp1, max_size);
                    if (ret == 0)
                    {
                        u8 sudoku_temp2[9 * 9];
                        memcpy(sudoku_temp2, sudoku, max_size);
                        sudoku_backtrack_min(sudoku_temp2, max_row, max_col, block_size);
                        assert(memcmp(sudoku_temp2, sudoku_temp, max_size) == 0);
                        break; // found it
                    }
                }
            }
            else {
                if (bcount > 387420489) {
                    printf("count: %d\n", bcount);
                    sudoku[i] = val;
                    char fname[256] = {};
                    sprintf(fname, "su_deadlock_%d_%d.txt", i, val);
                    write_sudoku_file(fname, sudoku, max_size, max_row);
                    sudoku[i] = 0;
                }
                
                ++backtrack_try_times;
            }
        } 
        else 
        {
            ++backtrack_try_times;
        }
        
        
    }
    
    sudoku_print(sudoku, max_size);
}

void run_tests(bool print_result)
{
    test("sudokus/s01a.txt", "solutions/s01a_s.txt", print_result);
    test("sudokus/s01b.txt", "solutions/s01b_s.txt", print_result);
    test("sudokus/s01c.txt", "solutions/s01c_s.txt", print_result);
    test("sudokus/s02a.txt", "solutions/s02a_s.txt", print_result);
    test("sudokus/s02b.txt", "solutions/s02b_s.txt", print_result);
    test("sudokus/s02c.txt", "solutions/s02c_s.txt", print_result);
    test("sudokus/s03a.txt", "solutions/s03a_s.txt", print_result);
    test("sudokus/s03b.txt", "solutions/s03b_s.txt", print_result);
    test("sudokus/s03c.txt", "solutions/s03c_s.txt", print_result);
    test("sudokus/s04a.txt", "solutions/s04a_s.txt", print_result);
    test("sudokus/s04b.txt", "solutions/s04b_s.txt", print_result);
    test("sudokus/s04c.txt", "solutions/s04c_s.txt", print_result);
    test("sudokus/s05a.txt", "solutions/s05a_s.txt", print_result);
    test("sudokus/s05b.txt", "solutions/s05b_s.txt", print_result);
    test("sudokus/s05c.txt", "solutions/s05c_s.txt", print_result);
    test("sudokus/s06a.txt", "solutions/s06a_s.txt", print_result);
    test("sudokus/s06b.txt", "solutions/s06b_s.txt", print_result);
    test("sudokus/s06c.txt", "solutions/s06c_s.txt", print_result);
    test("sudokus/s07a.txt", "solutions/s07a_s.txt", print_result);
    test("sudokus/s07b.txt", "solutions/s07b_s.txt", print_result);
    test("sudokus/s07c.txt", "solutions/s07c_s.txt", print_result);
    test("sudokus/s08a.txt", "solutions/s08a_s.txt", print_result);
    test("sudokus/s08b.txt", "solutions/s08b_s.txt", print_result);
    test("sudokus/s08c.txt", "solutions/s08c_s.txt", print_result);
    test("sudokus/s09a.txt", "solutions/s09a_s.txt", print_result);
    test("sudokus/s09b.txt", "solutions/s09b_s.txt", print_result);
    test("sudokus/s09c.txt", "solutions/s09c_s.txt", print_result);
    test("sudokus/s10a.txt", "solutions/s10a_s.txt", print_result);
    test("sudokus/s10b.txt", "solutions/s10b_s.txt", print_result);
    test("sudokus/s10c.txt", "solutions/s10c_s.txt", print_result);
    test("sudokus/s11a.txt", "solutions/s11a_s.txt", print_result);
    test("sudokus/s11b.txt", "solutions/s11b_s.txt", print_result);
    test("sudokus/s11c.txt", "solutions/s11c_s.txt", print_result);
    test("sudokus/s12a.txt", "solutions/s12a_s.txt", print_result);
    test("sudokus/s12b.txt", "solutions/s12b_s.txt", print_result);
    test("sudokus/s12c.txt", "solutions/s12c_s.txt", print_result);
    test("sudokus/s13a.txt", "solutions/s13a_s.txt", print_result);
    test("sudokus/s13b.txt", "solutions/s13b_s.txt", print_result);
    test("sudokus/s13c.txt", "solutions/s13c_s.txt", print_result);
    test("sudokus/s14a.txt", "solutions/s14a_s.txt", print_result);
    test("sudokus/s14b.txt", "solutions/s14b_s.txt", print_result);
    test("sudokus/s14c.txt", "solutions/s14c_s.txt", print_result);
    test("sudokus/s15a.txt", "solutions/s15a_s.txt", print_result);
    test("sudokus/s15b.txt", "solutions/s15b_s.txt", print_result);
    test("sudokus/s15c.txt", "solutions/s15c_s.txt", print_result);
    test("sudokus/s16.txt", "solutions/s16_s.txt", print_result);
}


void solve_data_file(char *fname)
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
        u8 sudoku[max_size * 2 + padding * 2] = {0};
        u16 sudoku_possible[max_size * 3] = {0};
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
            sudoku_print(sudoku, max_size);
            sudoku_fill_possible(sudoku, sudoku_possible, max_row, max_col, block_size);
            bool ret = sudoku_backtrack_heuristic(sudoku, sudoku_possible, max_row, max_col, block_size, &num_guess);
            
            //bool ret = sudoku_backtrack_min(sudoku, max_row, max_col, block_size, &num_guess);
            
            total_guess += num_guess;
            
            
            printf("Number of guess: %d\n", num_guess);
            sudoku_print(sudoku, max_size);
            fflush(stdout);
            //assert(ret == true);
            
            solved_count++;
            if (solved_count >30) 
                break;
            
            memset(sudoku_possible, 0, max_size * sizeof(u16));
            memset(sudoku, 0, max_size);
            assert(ret);
        }
        
        if (total_guess) {
            printf("Total guess/problem %lld/%d=%lld\n", total_guess, solved_count, total_guess/solved_count);
        }
        
        if (file_contents) free(file_contents);
    }
    
}
