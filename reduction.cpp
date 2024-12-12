#include <omp.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <tuple>

using std::array;
using std::cin;
using std::cout;
using std::get;
using std::make_tuple;
using std::string;
using std::tuple;
using std::vector;

array<array<int, 9>, 9> sudoku;
bool solved = false;

int checkrow(int row, int num, const array<array<int, 9>, 9> &sudoku)
{
    for (int column = 0; column < 9; column++)
        if (sudoku[row][column] == num)
            return 0;
    return 1;
}

int checkcolumn(int column, int num, const array<array<int, 9>, 9> &sudoku)
{
    for (int row = 0; row < 9; row++)
        if (sudoku[row][column] == num)
            return 0;
    return 1;
}

int checkgrid(int grid_row, int grid_column, int num, const array<array<int, 9>, 9> &sudoku)
{
    for (int row = grid_row * 3; row < (grid_row + 1) * 3; row++)
        for (int column = grid_column * 3; column < (grid_column + 1) * 3; column++)
            if (sudoku[row][column] == num)
                return 0;
    return 1;
}

void draw(const array<array<int, 9>, 9> &sudoku)
{
    for (int row = 0; row < 9; row++)
    {
        for (int column = 0; column < 9; column++)
        {
            cout << sudoku[row][column] << " ";
            if (column == 2 || column == 5)
                cout << "| ";
        }
        cout << "\n";
        if (row == 2 || row == 5)
            cout << "---------------------\n";
    }
}

tuple<bool, array<array<vector<int>, 9>, 9>> alternatives(const array<array<int, 9>, 9> &sudoku)
{
    array<array<vector<int>, 9>, 9> alt;
    for (int row = 0; row < 9; row++)
        for (int column = 0; column < 9; column++)
            if (sudoku[row][column] == 0)
            {
                for (int n = 1; n <= 9; n++)
                    if (checkrow(row, n, sudoku) && checkcolumn(column, n, sudoku) && checkgrid(row / 3, column / 3, n, sudoku))
                        alt[row][column].push_back(n);
                if (alt[row][column].empty())
                    return make_tuple(false, alt);
            }
    return make_tuple(true, alt);
}

bool fill(array<array<int, 9>, 9> &sudoku, array<array<vector<int>, 9>, 9> alt)
{
    bool flag = false;
    for (int row = 0; row < 9; row++)
        for (int column = 0; column < 9; column++)
            if (alt[row][column].size() == 1)
            {
                sudoku[row][column] = alt[row][column][0];
                alt[row][column].clear();
                flag = true;
            }
    return flag;
}

tuple<int, int> best(const array<array<vector<int>, 9>, 9> &alt)
{
    for (int i = 2; i <= 9; i++)
        for (int row = 0; row < 9; row++)
            for (int column = 0; column < 9; column++)
                if (alt[row][column].size() == i)
                    return make_tuple(row, column);
    return make_tuple(-1, -1);
}

void solve(array<array<int, 9>, 9> state, array<array<vector<int>, 9>, 9> alt)
{
    if (solved) // Check if already solved
        return;

    while (fill(state, alt))
    {
        auto val = alternatives(state);
        if (!get<0>(val))
            return;
        alt = get<1>(val);
    }

    auto pos = best(alt);
    int row = get<0>(pos);
    int column = get<1>(pos);

    if (row == -1 && column == -1)
    {
        #pragma omp critical // Protect shared resource
        {
            if (!solved)
            {
                sudoku = state;
                solved = true;
            }
        }
        return;
    }

    int local_solved = 0;

    #pragma omp parallel for reduction(|: local_solved) schedule(dynamic)
    for (int i = 0; i < alt[row][column].size(); i++)
    {
        if (solved)
            continue;

        array<array<int, 9>, 9> local_state = state; // Thread-local copy
        local_state[row][column] = alt[row][column][i];

        auto val = alternatives(local_state);
        if (get<0>(val))
        {
            solve(local_state, get<1>(val)); // Recursive call
            local_solved |= solved;
        }
    }

    #pragma omp critical
    solved |= local_solved; // Update the global solved flag
}

int main()
{
    cout << "To see an example, press 'y'. To enter your own sudoku, press 'n'.\n";
    string choice;
    cin >> choice;

    if (choice == "y")
    {
        sudoku = {{
            {0, 0, 0, 6, 2, 0, 0, 0, 3},
            {0, 7, 0, 0, 8, 9, 0, 1, 0},
            {0, 0, 3, 1, 0, 0, 6, 0, 0},
            {0, 0, 4, 0, 0, 0, 7, 6, 0},
            {0, 0, 0, 0, 4, 0, 0, 0, 0},
            {0, 9, 2, 0, 0, 0, 5, 0, 0},
            {0, 0, 6, 0, 0, 3, 1, 0, 0},
            {0, 2, 0, 5, 9, 0, 0, 8, 0},
            {3, 0, 0, 0, 6, 8, 0, 0, 0},
        }};
    }
    else if (choice == "n")
    {
        cout << "Enter your Sudoku (row by row, 0 for empty cells):\n";
        for (int row = 0; row < 9; row++)
            for (int column = 0; column < 9; column++)
                cin >> sudoku[row][column];
    }

    cout << "Sudoku to be solved:\n";
    draw(sudoku);

    double start_time = omp_get_wtime();

    auto val = alternatives(sudoku);
    if (get<0>(val))
        solve(sudoku, get<1>(val));

    double end_time = omp_get_wtime();

    if (solved)
    {
        cout << "Sudoku solved:\n";
        draw(sudoku);
    }
    else
    {
        cout << "Sudoku could not be solved.\n";
    }

    cout << "Time taken: " << (end_time - start_time) << " seconds.\n";

    return 0;
}