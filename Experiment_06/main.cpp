// Experiment 6: Function overloading and operator overloading
#include <iostream>
using namespace std;

// function overloading: same name, different parameters
int add(int a, int b) { return a + b; }
double add(double a, double b) { return a + b; }
int add(int a, int b, int c) { return a + b + c; }

class Complex {
    int real, imag;
public:
    Complex(int r = 0, int i = 0) { real = r; imag = i; }
    // operator overloading
    Complex operator+(Complex c) { return Complex(real + c.real, imag + c.imag); }
    Complex operator-(Complex c) { return Complex(real - c.real, imag - c.imag); }
    void show() { cout << real << " + " << imag << "i" << endl; }
};

int main() {
    cout << add(2, 3) << endl;          // int version
    cout << add(2.5, 3.5) << endl;      // double version
    cout << add(1, 2, 3) << endl;       // three-argument version

    Complex c1(3, 4), c2(1, 2);
    Complex sum = c1 + c2;
    Complex diff = c1 - c2;
    sum.show();
    diff.show();
    return 0;
}
