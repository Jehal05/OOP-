# OOP-

C++ Object-Oriented Programming lab work: ten experiments and one mini project.

## Branches
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
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

Open a branch to see its own README, with the aim, how the code works, run instructions, sample output and viva questions.

## Experiments at a glance
| # | Topic | Main idea |
|---|---|---|
| 1 | Data types, constants, variables, operators, I/O | The basics of a C++ program |
| 2 | Class hierarchy (Bank Account) | Access specifiers and member functions |
| 3 | Constructors and the Rule of 5 | Default, parameterized, copy, move; Valgrind check |
| 4 | Inheritance | Single and multilevel (University) |
| 5 | Runtime polymorphism | Virtual functions and an abstract `Shape` |
| 6 | Compile-time polymorphism | Function and operator overloading (`Complex`) |
| 7 | Templates | Generic `Stack` and `Queue` |
| 8 | STL | `vector`, `map`, `sort`, `accumulate`, `find` |
| 9 | Exceptions | Custom exceptions for banking |
| 10 | File handling | CSV with `fstream`, `seekg`/`seekp`/`tellg`/`tellp` |

## Mini project: Nova Bank
A banking and management system that uses all of the above together: abstract classes, inheritance, polymorphic fraud rules, templates (`Repository<T>`), STL, custom exceptions and CSV persistence.

- `mini-project`: the terminal application, with screenshots, diagrams and a viva Q&A sheet.
- `web-ui`: the same engine behind a browser interface at `http://localhost:8080`.

## Compile any experiment
```bash
git checkout experiment-01        # or experiment-02 ... experiment-10
cd Experiment_01
g++ -std=c++17 main.cpp -o exp && ./exp
```
On Windows with MinGW, use `g++ -std=c++17 main.cpp -o exp.exe` then `.\exp.exe` in PowerShell.

## Author
Jehal05
