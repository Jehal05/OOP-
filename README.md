# Experiment 1: Data types, constants, variables, operators and I/O

Object-Oriented Programming lab, C++ (experiment-01 branch). Source code: [`Experiment_01/main.cpp`](Experiment_01/main.cpp)

## Aim
Write a C++ program that uses the basic data types, a constant, variables, operators and console input/output.

## Concepts covered
- Data types: `int`, `float`, `double`, `char`, `bool`
- `const` keyword
- Arithmetic, relational and logical operators
- `cin` and `cout`, `sizeof`

## How the code works
1. Variables of each basic type are declared and given a value.
2. `const int MAX_MARKS` cannot be changed after it is set.
3. The program reads two numbers with `cin` and prints their sum, difference, product, quotient and remainder.
4. Relational (`>`) and logical (`&&`) operators print `1` for true and `0` for false.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_01
g++ -std=c++17 -Wall -Wextra main.cpp -o exp01
./exp01
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_01
g++ -std=c++17 -Wall -Wextra main.cpp -o exp01.exe
.\exp01.exe
```

The program asks for two numbers; the output below used `7 2`.

## Sample output
```text
Enter two numbers: Sum        : 9
Difference : 5
Product    : 14
Quotient   : 3
Remainder  : 1
a > b      : 1
a > 0 && b > 0 : 1
Age = 20, Height = 5.8, Pi = 3.14159
Grade = A, Passed = 1, Max marks = 100
Size of int = 4 bytes
```

## Viva questions
**Q1. What is the difference between `float` and `double`?**  `double` has about twice the precision of `float` (usually 8 bytes against 4).

**Q2. What does `const` do?**  It makes a variable read-only, so the compiler rejects any later change.

**Q3. What does `%` give?**  The remainder of integer division, for example `7 % 2` is `1`.

**Q4. What does `cout << (a > b)` print?**  `1` if the condition is true, `0` if it is false.

## Other branches in this repository
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O (this branch) |
| `experiment-02` | Experiment 2: Class hierarchy |
| `experiment-03` | Experiment 3: Constructors and Rule of 5 |
| `experiment-04` | Experiment 4: Inheritance |
| `experiment-05` | Experiment 5: Runtime polymorphism |
| `experiment-06` | Experiment 6: Overloading |
| `experiment-07` | Experiment 7: Templates |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
