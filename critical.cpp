#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <chrono> // Include chrono for timing

using std::cout;
using std::cin;
using std::string;
using std::to_string;
using std::vector;
using std::array;
using std::tuple;
using std::make_tuple;
using std::get;

array<array<int, 9>, 9> sudoku;
bool solved = false;

int checkrow(int row, int num, array<array<int, 9>, 9> sudoku)
{
    for (int column = 0; column < 9; column++)
        if (sudoku[row][column] == num)
            return 0;
    return 1;
}

int checkcolumn(int column, int num, array<array<int, 9>, 9> sudoku)
{
    for (int row = 0; row < 9; row++)
        if (sudoku[row][column] == num)
            return 0;
    return 1;
}

int checkgrid(int grid_row, int grid_column, int num, array<array<int, 9>, 9> sudoku)
{
    for (int row = grid_row * 3; row < (grid_row + 1) * 3; row++)
        for (int column = grid_column * 3; column < (grid_column + 1) * 3; column++)
            if (sudoku[row][column] == num)
                return 0;
    return 1;
}

void draw(array<array<int, 9>, 9> sudoku)
{
    for (int row = 0; row < 9; row++)
    {
        for (int column = 0; column < 9; column++)
        {
            cout << to_string(sudoku[row][column]) + " ";
            if (column == 2 || column == 5)
                cout << "| ";
        }
        cout << "\n";
        if (row == 2 || row == 5)
            cout << "---------------------\n";
    }
}

tuple<bool, array<array<vector<int>, 9>, 9>> alternatives(array<array<int, 9>, 9> sudoku)
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
            if (alt[row][column].size() == 1 && alt[row][column][0] != 0)
            {
                sudoku[row][column] = alt[row][column][0];
                alt[row][column].clear();
                flag = true;
            }
    return flag;
}

tuple<int, int> best(array<array<vector<int>, 9>, 9> alt)
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

    tuple<bool, array<array<vector<int>, 9>, 9>> val;

    while (fill(state, alt))
    {
        val = alternatives(state);
        if (!get<0>(val))
            return;
        else
            alt = get<1>(val);
    }

    tuple<int, int> pos = best(alt);
    int row = get<0>(pos);
    int column = get<1>(pos);

    if (row == -1 && column == -1)
    {
        // If no empty cells remain and sudoku is valid
        #pragma omp critical // Protect shared variable update
        {
            if (!solved) // Double-check within critical section
            {
                sudoku = state;
                solved = true;
            }
        }
    }
    else
    {
        #pragma omp parallel for shared(sudoku, solved) // Use OpenMP parallelization
        for (int i = 0; i < alt[row][column].size(); i++)
        {
            if (solved) // Check flag to avoid unnecessary computation
                continue;

            array<array<int, 9>, 9> local_state = state; // Create a thread-local copy of the state
            local_state[row][column] = alt[row][column][i];
            val = alternatives(local_state);

            if (get<0>(val))
            {
                #pragma omp critical // Protect recursive solve
                {
                    if (!solved) // Re-check flag inside critical section
                        solve(local_state, get<1>(val));
                }
            }
        }
    }
}

int main()
{
    int row, column;
    tuple<bool, array<array<vector<int>, 9>, 9>> val;

    string num;
    cout << "To see an example, press 'y'. To enter your own sudoku, press 'n'.\n";
    cin >> num;

    if (num == "y")
    {
        // Example Sudoku
        array<array<int, 9>, 9> d = {
            0, 0, 0, 6, 2, 0, 0, 0, 3,
            0, 7, 0, 0, 8, 9, 0, 1, 0,
            0, 0, 3, 1, 0, 0, 6, 0, 0,
            0, 0, 4, 0, 0, 0, 7, 6, 0,
            0, 0, 0, 0, 4, 0, 0, 0, 0,
            0, 9, 2, 0, 0, 0, 5, 0, 0,
            0, 0, 6, 0, 0, 3, 1, 0, 0,
            0, 2, 0, 5, 9, 0, 0, 8, 0,
            3, 0, 0, 0, 6, 8, 0, 0, 0
        };
        sudoku = d;
    }
    else if (num == "n")
    {
        cout << "Please enter your sudoku, filling unknown cells with 0. Enter each number followed by pressing `Enter`. Fill the Sudoku row by row, starting from the top-left cell.\n";
        for (row = 0; row < 9; row++)
            for (column = 0; column < 9; column++)
            {
                cin >> num;
                if (stoi(num) >= 0 && stoi(num) <= 9)
                    sudoku[row][column] = stoi(num);
                else
                {
                    cout << "Invalid number.";
                    return 0;
                }
            }
    }
    else
    {
        cout << "Invalid character.";
        return 0;
    }

    cout << "Sudoku to be solved:\n";
    draw(sudoku);

    // Start timer using chrono
    auto start_time = std::chrono::steady_clock::now();

    cout << "Solving...\n";
    val = alternatives(sudoku);
    if (get<0>(val))
        solve(sudoku, get<1>(val));

    // End timer
    auto end_time = std::chrono::steady_clock::now();

    if (solved)
    {
        cout << "Sudoku solved!\n";
        draw(sudoku);
    }
    else
        cout << "Sudoku could not be solved.\n";

    // Calculate and print elapsed time
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    cout << "Time taken to solve the Sudoku: " << elapsed_seconds.count() << " seconds.\n";

    return 0;
}