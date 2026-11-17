// Experiment 10: Reading and writing a CSV file with fstream, and file pointers
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
    // write a CSV file
    ofstream out("students.csv");
    out << "1,Asha,85\n";
    out << "2,Ravi,72\n";
    out << "3,Meera,91\n";
    cout << "Write position after writing: " << out.tellp() << endl;   // tellp
    out.close();

    // read it back
    ifstream in("students.csv");
    string line;
    cout << "File contents:" << endl;
    while (getline(in, line)) cout << line << endl;

    // file pointers: go back to the start and read the first line again
    in.clear();
    in.seekg(0, ios::beg);                                              // seekg
    getline(in, line);
    cout << "First line again: " << line << endl;
    cout << "Read position now: " << in.tellg() << endl;                // tellg
    in.close();

    // overwrite one byte in the middle using seekp
    fstream f("students.csv", ios::in | ios::out);
    f.seekp(2);                                                         // seekp
    f.put('X');
    f.close();
    return 0;
}
