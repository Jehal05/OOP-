# Experiment 5: Runtime polymorphism with virtual functions and an abstract class

Object-Oriented Programming lab, C++ (experiment-05 branch). Source code: [`Experiment_05/main.cpp`](Experiment_05/main.cpp)

## Aim
Show runtime polymorphism using virtual functions and an abstract base class.

## Concepts covered
- Virtual functions
- Pure virtual function and abstract class
- Base-class pointers to derived objects
- Virtual destructor

## How the code works
1. `Shape` has a pure virtual function `area() = 0`, so it cannot be instantiated.
2. `Circle` and `Rectangle` override `area()` and `name()`.
3. An array of `Shape*` holds both kinds of shape; the call `shapes[i]->area()` runs the right version at run time.
4. The virtual destructor makes `delete shapes[i]` clean up the real object type.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_05
g++ -std=c++17 -Wall -Wextra main.cpp -o exp05
./exp05
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_05
g++ -std=c++17 -Wall -Wextra main.cpp -o exp05.exe
.\exp05.exe
```

## Sample output
```text
Circle area = 78.5397
Rectangle area = 24
```

## Viva questions
**Q1. What is a pure virtual function?**  A virtual function with no body, written `= 0`; it makes the class abstract.

**Q2. Why can't we create an object of an abstract class?**  Because it has at least one function with no implementation.

**Q3. What is runtime (dynamic) binding?**  The function to call is chosen while the program runs, based on the real object type.

**Q4. Why make the destructor virtual?**  So deleting through a base pointer calls the derived destructor too.

## Other branches in this repository
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
| `experiment-02` | Experiment 2: Class hierarchy |
| `experiment-03` | Experiment 3: Constructors and Rule of 5 |
| `experiment-04` | Experiment 4: Inheritance |
| `experiment-05` | Experiment 5: Runtime polymorphism (this branch) |
| `experiment-06` | Experiment 6: Overloading |
| `experiment-07` | Experiment 7: Templates |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
