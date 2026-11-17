# Experiment 4: Single and multilevel inheritance (University management)

Object-Oriented Programming lab, C++ (experiment-04 branch). Source code: [`Experiment_04/main.cpp`](Experiment_04/main.cpp)

## Aim
Demonstrate single and multilevel inheritance with a simple university model.

## Concepts covered
- Single inheritance
- Multilevel inheritance
- `protected` members
- Constructor chaining with initializer lists

## How the code works
1. `Student` inherits from `Person` (single inheritance).
2. `GradStudent` inherits from `Student`, which inherits from `Person` (multilevel inheritance).
3. Each constructor passes its share of the data up to the parent constructor.
4. `showGrad()` calls `showStudent()`, which calls `showPerson()`, so one call prints the whole chain.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_04
g++ -std=c++17 -Wall -Wextra main.cpp -o exp04
./exp04
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_04
g++ -std=c++17 -Wall -Wextra main.cpp -o exp04.exe
.\exp04.exe
```

## Sample output
```text
--- Single inheritance ---
Name: Asha, Age: 19
Roll no: 101
--- Multilevel inheritance ---
Name: Ravi, Age: 24
Roll no: 201
Thesis: Machine Learning
```

## Viva questions
**Q1. What is single inheritance?**  A class derived from exactly one base class.

**Q2. What is multilevel inheritance?**  A chain where a class is derived from a class that is itself derived from another.

**Q3. In what order are constructors called?**  Base class first, then the derived classes, down the chain.

**Q4. Why use `protected`?**  It lets derived classes use the data while hiding it from the outside.

## Other branches in this repository
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
| `experiment-02` | Experiment 2: Class hierarchy |
| `experiment-03` | Experiment 3: Constructors and Rule of 5 |
| `experiment-04` | Experiment 4: Inheritance (this branch) |
| `experiment-05` | Experiment 5: Runtime polymorphism |
| `experiment-06` | Experiment 6: Overloading |
| `experiment-07` | Experiment 7: Templates |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
