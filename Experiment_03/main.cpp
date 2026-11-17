// Experiment 3: Constructors (default, parameterized, copy, move) and the Rule of 5
#include <iostream>
#include <cstring>
using namespace std;

class Student {
    char* name;
    int marks;
public:
    // default constructor
    Student() {
        name = new char[1];
        name[0] = '\0';
        marks = 0;
        cout << "Default constructor" << endl;
    }
    // parameterized constructor
    Student(const char* n, int m) {
        name = new char[strlen(n) + 1];
        strcpy(name, n);
        marks = m;
        cout << "Parameterized constructor" << endl;
    }
    // copy constructor (deep copy)
    Student(const Student& other) {
        name = new char[strlen(other.name) + 1];
        strcpy(name, other.name);
        marks = other.marks;
        cout << "Copy constructor" << endl;
    }
    // move constructor (steal the pointer)
    Student(Student&& other) noexcept {
        name = other.name;
        marks = other.marks;
        other.name = nullptr;
        cout << "Move constructor" << endl;
    }
    // copy assignment
    Student& operator=(const Student& other) {
        if (this != &other) {
            delete[] name;
            name = new char[strlen(other.name) + 1];
            strcpy(name, other.name);
            marks = other.marks;
        }
        cout << "Copy assignment" << endl;
        return *this;
    }
    // move assignment
    Student& operator=(Student&& other) noexcept {
        if (this != &other) {
            delete[] name;
            name = other.name;
            marks = other.marks;
            other.name = nullptr;
        }
        cout << "Move assignment" << endl;
        return *this;
    }
    // destructor
    ~Student() {
        delete[] name;
        cout << "Destructor" << endl;
    }
    void show() const {
        cout << (name ? name : "(moved)") << " - " << marks << endl;
    }
};

int main() {
    Student s1;
    Student s2("Asha", 90);
    Student s3 = s2;             // copy constructor
    Student s4 = move(s2);       // move constructor
    s1 = s3;                     // copy assignment
    s1 = Student("Ravi", 80);    // move assignment
    s1.show();
    s3.show();
    s4.show();
    return 0;
}
