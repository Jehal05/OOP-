// Experiment 1: Data types, constants, variables, operators and input/output
#include <iostream>
using namespace std;

int main() {
    // data types
    int age = 20;
    float height = 5.8f;
    double pi = 3.14159;
    char grade = 'A';
    bool passed = true;

    // constants
    const int MAX_MARKS = 100;

    // input
    int a, b;
    cout << "Enter two numbers: ";
    cin >> a >> b;

    // operators
    cout << "Sum        : " << a + b << endl;
    cout << "Difference : " << a - b << endl;
    cout << "Product    : " << a * b << endl;
    cout << "Quotient   : " << a / b << endl;
    cout << "Remainder  : " << a % b << endl;
    cout << "a > b      : " << (a > b) << endl;
    cout << "a > 0 && b > 0 : " << (a > 0 && b > 0) << endl;

    // output of the variables
    cout << "Age = " << age << ", Height = " << height << ", Pi = " << pi << endl;
    cout << "Grade = " << grade << ", Passed = " << passed << ", Max marks = " << MAX_MARKS << endl;
    cout << "Size of int = " << sizeof(int) << " bytes" << endl;
    return 0;
}
