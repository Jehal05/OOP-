# Experiment 6: Function overloading and operator overloading

Object-Oriented Programming lab, C++ (experiment-06 branch). Source code: [`Experiment_06/main.cpp`](Experiment_06/main.cpp)

## Aim
Show compile-time polymorphism using function overloading and operator overloading.

## Concepts covered
- Function overloading
- Operator overloading (`+`, `-`)
- Default arguments in a constructor

## How the code works
1. Three versions of `add()` differ in parameter types or count; the compiler picks one at compile time.
2. `Complex` stores a real and an imaginary part.
3. `operator+` and `operator-` let you write `c1 + c2` for complex numbers.
4. The constructor has default arguments, so `Complex()` makes `0 + 0i`.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_06
g++ -std=c++17 -Wall -Wextra main.cpp -o exp06
./exp06
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_06
g++ -std=c++17 -Wall -Wextra main.cpp -o exp06.exe
.\exp06.exe
```

## Sample output
```text
5
6
6
4 + 6i
2 + 2i
```

## Viva questions
**Q1. What is function overloading?**  Several functions with the same name but different parameter lists.

**Q2. Is the return type enough to overload?**  No, the parameter list must differ.

**Q3. Which operators cannot be overloaded?**  `::`, `.`, `.*`, `?:` and `sizeof`.

**Q4. Why is this called compile-time polymorphism?**  The compiler decides which function to call before the program runs.

## Other branches in this repository
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
| `experiment-02` | Experiment 2: Class hierarchy |
| `experiment-03` | Experiment 3: Constructors and Rule of 5 |
| `experiment-04` | Experiment 4: Inheritance |
| `experiment-05` | Experiment 5: Runtime polymorphism |
| `experiment-06` | Experiment 6: Overloading (this branch) |
| `experiment-07` | Experiment 7: Templates |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
