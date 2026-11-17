# Experiment 9: Custom exception handling for banking transactions

Object-Oriented Programming lab, C++ (experiment-09 branch). Source code: [`Experiment_09/main.cpp`](Experiment_09/main.cpp)

## Aim
Create custom exceptions and handle them in a banking program.

## Concepts covered
- `try`, `throw`, `catch`
- Custom exception classes derived from `std::exception`
- Overriding `what()`

## How the code works
1. `InsufficientFunds` and `InvalidAmount` both derive from `std::exception` and override `what()`.
2. `withdraw()` throws `InvalidAmount` for zero or negative amounts and `InsufficientFunds` if the balance is too low.
3. `main()` wraps the calls in `try` blocks and prints `e.what()` in the `catch`.
4. The program keeps running after each error instead of crashing.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_09
g++ -std=c++17 -Wall -Wextra main.cpp -o exp09
./exp09
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_09
g++ -std=c++17 -Wall -Wextra main.cpp -o exp09.exe
.\exp09.exe
```

## Sample output
```text
Withdrawn 300, balance = 700
Error: Invalid amount
Error: Insufficient funds
```

## Viva questions
**Q1. Why use custom exceptions?**  They give clear, specific error types that callers can handle separately.

**Q2. What does `what()` return?**  A text description of the error.

**Q3. Why catch by reference?**  It avoids copying and keeps the real exception type.

**Q4. What happens if an exception is never caught?**  The program calls `std::terminate` and stops.

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
| `experiment-09` | Experiment 9: Exceptions (this branch) |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |
