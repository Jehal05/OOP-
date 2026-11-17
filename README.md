# Experiment 2: Class hierarchy for a real-world entity (Bank Account)

Object-Oriented Programming lab, C++ (experiment-02 branch). Source code: [`Experiment_02/main.cpp`](Experiment_02/main.cpp)

## Aim
Create a class hierarchy for a real-world entity and show access specifiers and member functions.

## Concepts covered
- Classes and objects
- Access specifiers: `private`, `protected`, `public`
- Member functions and constructors
- Inheritance (`SavingsAccount` extends `Account`)

## How the code works
1. `Account` keeps `balance` private, so only its own methods can change it.
2. `name` is protected, so the derived class `SavingsAccount` can read it, but outside code cannot.
3. `deposit`, `withdraw`, `getBalance` and `show` are public member functions.
4. `SavingsAccount` adds a `rate` and an `addInterest()` function built on the base class.

## How to run

**Linux / macOS / WSL**
```bash
cd Experiment_02
g++ -std=c++17 -Wall -Wextra main.cpp -o exp02
./exp02
```

**Windows (PowerShell, with MinGW g++)**
```powershell
cd Experiment_02
g++ -std=c++17 -Wall -Wextra main.cpp -o exp02.exe
.\exp02.exe
```

## Sample output
```text
Asha has Rs 5500
Account holder: Ravi
Ravi has Rs 10400
```

## Viva questions
**Q1. Why is `balance` private?**  Encapsulation: outside code must go through `deposit` and `withdraw`, which check the rules.

**Q2. What is the difference between `private` and `protected`?**  Protected members are also visible inside derived classes; private ones are not.

**Q3. How does the derived class call the base constructor?**  With an initializer list: `SavingsAccount(...) : Account(n, b)`.

**Q4. What is a member function?**  A function declared inside a class that works on that class's data.

## Other branches in this repository
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
| `experiment-02` | Experiment 2: Class hierarchy (this branch) |
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
