# Experiment 10: CSV file handling with fstream and file pointers

Object-Oriented Programming lab, C++ (experiment-10 branch). Source code: [`Experiment_10/main.cpp`](Experiment_10/main.cpp)

## Aim
Write and read a CSV file with `fstream`, and use the file pointer functions.

## Concepts covered
- `ofstream`, `ifstream`, `fstream`
- `getline`
- File pointers: `tellp`, `tellg`, `seekg`, `seekp`

## How the code works
1. `ofstream` writes three student records as comma-separated lines to `students.csv`.
2. `tellp()` reports the write position (31 bytes after three lines).
3. `ifstream` reads the file back line by line with `getline`.
4. `seekg(0)` jumps back to the start, `tellg()` shows the read position, and `seekp(2)` with `put('X')` overwrites one byte in place.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_10
g++ -std=c++17 -Wall -Wextra main.cpp -o exp10
./exp10
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_10
g++ -std=c++17 -Wall -Wextra main.cpp -o exp10.exe
.\exp10.exe
```

The program creates `students.csv` in the folder where you run it.

## Sample output
```text
Write position after writing: 31
File contents:
1,Asha,85
2,Ravi,72
3,Meera,91
First line again: 1,Asha,85
Read position now: 10
```

## Viva questions
**Q1. What does CSV stand for?**  Comma-Separated Values, a plain text table format.

**Q2. `seekg` vs `seekp`?**  `seekg` moves the read pointer; `seekp` moves the write pointer.

**Q3. Why call `in.clear()` before `seekg`?**  After reaching the end of file, the error flags must be cleared or the seek is ignored.

**Q4. `ifstream` vs `fstream`?**  `ifstream` only reads, `fstream` can read and write.

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
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling (this branch) |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
