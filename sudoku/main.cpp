#include "sudoku.cpp"

int main()
{
    solve_data_file("data/puzzles2_17_clue");
    
#if 0
    for (int i = 0; i < 10; ++i)
        run_tests(false);
#endif
    //test("sudokus/s01a.txt", "solutions/s01a_s.txt");
    
#if 0
    u8 su_deadlock[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 8, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 2, 
        0, 9, 0, 0, 6, 0, 0, 0, 5, 
        0, 0, 0, 9, 0, 0, 2, 0, 0, 
        0, 0, 0, 0, 0, 3, 0, 0, 0, 
        0, 6, 0, 0, 0, 7, 0, 5, 0, 
        3, 0, 0, 0, 2, 0, 0, 0, 0, 
        0, 0, 0, 0, 9, 0, 0, 1, 0
    };
    const int max_size = 9 * 9;
    sudoku_print(su_deadlock, max_size);
    
    u16 sudoku_possible[max_size] = {};
    sudoku_fill_possible(su_deadlock, sudoku_possible, 9, 9, 3);
    sudoku_backtrack_with_possible(su_deadlock, sudoku_possible, 0, 9, 9, 3);
    
    
    u32 bcount = 0;
    sudoku_backtrack(su_deadlock, 0, 9, 9, 3, &bcount);
    //sudoku_backtrack_r(su_deadlock, 0, 9, 9, 3, &bcount);
    //sudoku_backtrack_min(su_deadlock, 9, 9, 3);
    sudoku_print(su_deadlock, max_size);
#endif
    
#if 0
    int i = 0;
    while(i < 1) {
        ++i;
        generate_sudoku(50, 9, 9, 3);
    }
#endif
}
