# Experiment 7: Generic Stack and Queue using templates

Object-Oriented Programming lab, C++ (experiment-07 branch). Source code: [`Experiment_07/main.cpp`](Experiment_07/main.cpp)

## Aim
Create a generic Stack and Queue with templates and test them with different data types.

## Concepts covered
- Class templates (`template <class T>`)
- Generic programming
- Stack (LIFO) and Queue (FIFO)

## How the code works
1. `Stack<T>` uses an array and a `top` index: `push` adds, `pop` removes the last item added.
2. `Queue<T>` uses `front` and `rear` indexes: `enqueue` adds at the back, `dequeue` removes from the front.
3. The same code is used for `int`, `string` and `float` by writing `Stack<int>`, `Stack<string>` and `Queue<float>`.
4. The output shows LIFO order for the stacks (3 2 1) and FIFO order for the queue (1.5 2.5 3.5).

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_07
g++ -std=c++17 -Wall -Wextra main.cpp -o exp07
./exp07
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_07
g++ -std=c++17 -Wall -Wextra main.cpp -o exp07.exe
.\exp07.exe
```

## Sample output
```text
Int stack: 3 2 1 
String stack: two one 
Float queue: 1.5 2.5 3.5
```

## Viva questions
**Q1. What is a template?**  A blueprint that lets one class or function work with any data type.

**Q2. Stack vs queue?**  A stack is last-in first-out; a queue is first-in first-out.

**Q3. When does the compiler create the real class?**  When you use it with a type, like `Stack<int>` (template instantiation).

**Q4. Where must template code be defined?**  Usually in the header or the same file, because the compiler needs the full code to instantiate it.

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
| `experiment-07` | Experiment 7: Templates (this branch) |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
