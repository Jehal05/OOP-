# Verify with Valgrind
```
g++ -g -std=c++17 main.cpp -o exp03
valgrind --leak-check=full ./exp03
```
Result: all heap blocks freed, 0 errors.
