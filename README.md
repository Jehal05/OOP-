# Experiment 8: STL containers and algorithms

Object-Oriented Programming lab, C++ (experiment-08 branch). Source code: [`Experiment_08/main.cpp`](Experiment_08/main.cpp)

## Aim
Use STL containers and algorithms to process a small set of student marks.

## Concepts covered
- `vector` and `map`
- Algorithms: `sort`, `find`, `accumulate`, `max_element`
- Iterators and range-based `for`

## How the code works
1. A `vector<int>` holds the marks and `sort()` orders them.
2. `accumulate()` adds them up, `max_element()` finds the highest, and `find()` searches for a value.
3. A `map<string,int>` links each student name to a mark and keeps names in alphabetical order.
4. Iterators (`begin()`, `end()`) tell each algorithm which range to work on.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_08
g++ -std=c++17 -Wall -Wextra main.cpp -o exp08
./exp08
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_08
g++ -std=c++17 -Wall -Wextra main.cpp -o exp08.exe
.\exp08.exe
```

## Sample output
```text
Sorted marks: 60 72 78 85 91 
Total = 386, Average = 77
Highest = 91
85 found
Asha : 85
Meera : 91
Ravi : 72
```

## Viva questions
**Q1. vector vs array?**  A vector grows automatically and knows its size; a plain array has a fixed size.

**Q2. What is a map?**  A container of key-value pairs, sorted by key.

**Q3. What is an iterator?**  An object that points to an element of a container and moves through it.

**Q4. What header gives `accumulate`?**  `<numeric>`.

## Other branches in this repository
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
| `experiment-08` | Experiment 8: STL (this branch) |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
