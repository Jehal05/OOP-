// Experiment 4: Single and multilevel inheritance (University management)
#include <iostream>
#include <string>
using namespace std;

class Person {
protected:
    string name;
    int age;
public:
    Person(string n, int a) { name = n; age = a; }
    void showPerson() { cout << "Name: " << name << ", Age: " << age << endl; }
};

// single inheritance: Person -> Student
class Student : public Person {
protected:
    int rollNo;
public:
    Student(string n, int a, int r) : Person(n, a) { rollNo = r; }
    void showStudent() {
        showPerson();
        cout << "Roll no: " << rollNo << endl;
    }
};

// multilevel inheritance: Person -> Student -> GradStudent
class GradStudent : public Student {
    string thesis;
public:
    GradStudent(string n, int a, int r, string t) : Student(n, a, r) { thesis = t; }
    void showGrad() {
        showStudent();
        cout << "Thesis: " << thesis << endl;
    }
};

int main() {
    cout << "--- Single inheritance ---" << endl;
    Student s("Asha", 19, 101);
    s.showStudent();

    cout << "--- Multilevel inheritance ---" << endl;
    GradStudent g("Ravi", 24, 201, "Machine Learning");
    g.showGrad();
    return 0;
}
