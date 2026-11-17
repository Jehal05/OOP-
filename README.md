# Experiment 3: All types of constructors and the Rule of 5 (Valgrind-checked)

Object-Oriented Programming lab, C++ (experiment-03 branch). Source code: [`Experiment_03/main.cpp`](Experiment_03/main.cpp)

## Aim
Show default, parameterized, copy and move constructors, plus copy/move assignment and the destructor, and check for memory leaks with Valgrind.

## Concepts covered
- Default, parameterized, copy and move constructors
- Copy assignment and move assignment
- Destructor
- Deep copy vs shallow copy
- Rule of 5 and `delete[]`
- Valgrind

## How the code works
1. `Student` owns heap memory (`char* name`), so it needs all five special functions.
2. The copy constructor and copy assignment make a deep copy, so two objects never share one pointer.
3. The move constructor and move assignment steal the pointer and set the source to `nullptr`.
4. The destructor frees the memory with `delete[]`; deleting `nullptr` is safe, so moved-from objects are fine.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_03
g++ -std=c++17 -Wall -Wextra main.cpp -o exp03
./exp03
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_03
g++ -std=c++17 -Wall -Wextra main.cpp -o exp03.exe
.\exp03.exe
```

## Sample output
```text
Default constructor
Parameterized constructor
Copy constructor
Move constructor
Copy assignment
Parameterized constructor
Move assignment
Destructor
Ravi - 80
Asha - 90
Asha - 90
Destructor
Destructor
Destructor
Destructor
```

### Check for memory leaks
```bash
g++ -g main.cpp -o exp03
valgrind --leak-check=full ./exp03
```
Expected result: all heap blocks freed, 0 errors.

## Viva questions
**Q1. What is the Rule of 5?**  If a class needs a destructor, copy constructor, copy assignment, move constructor or move assignment, it usually needs all five.

**Q2. Deep copy vs shallow copy?**  A deep copy duplicates the heap data; a shallow copy only copies the pointer, so two objects would free the same memory.

**Q3. Why set `other.name = nullptr` in a move?**  So the moved-from object's destructor does not free memory that now belongs to the new object.

**Q4. When is the copy constructor called?**  When an object is created from another one, like `Student s3 = s2;`.

## Other branches in this repository
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
| `experiment-02` | Experiment 2: Class hierarchy |
| `experiment-03` | Experiment 3: Constructors and Rule of 5 (this branch) |
| `experiment-04` | Experiment 4: Inheritance |
| `experiment-05` | Experiment 5: Runtime polymorphism |
| `experiment-06` | Experiment 6: Overloading |
| `experiment-07` | Experiment 7: Templates |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
