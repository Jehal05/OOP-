// Experiment 5: Runtime polymorphism with virtual functions and an abstract base class
#include <iostream>
using namespace std;

class Shape {                       // abstract class
public:
    virtual double area() = 0;      // pure virtual function
    virtual void name() { cout << "Shape"; }
    virtual ~Shape() {}
};

class Circle : public Shape {
    double r;
public:
    Circle(double r) { this->r = r; }
    double area() { return 3.14159 * r * r; }
    void name() { cout << "Circle"; }
};

class Rectangle : public Shape {
    double l, b;
public:
    Rectangle(double l, double b) { this->l = l; this->b = b; }
    double area() { return l * b; }
    void name() { cout << "Rectangle"; }
};

int main() {
    Shape* shapes[2];
    shapes[0] = new Circle(5);
    shapes[1] = new Rectangle(4, 6);

    for (int i = 0; i < 2; i++) {
        shapes[i]->name();                       // the right version is chosen at run time
        cout << " area = " << shapes[i]->area() << endl;
    }

    for (int i = 0; i < 2; i++) delete shapes[i];
    return 0;
}
