// Experiment 8: STL containers (vector, map) and algorithms (sort, find, accumulate)
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <string>
using namespace std;

int main() {
    // vector of marks
    vector<int> marks = {72, 85, 60, 91, 78};

    sort(marks.begin(), marks.end());
    cout << "Sorted marks: ";
    for (int m : marks) cout << m << " ";
    cout << endl;

    int total = accumulate(marks.begin(), marks.end(), 0);
    cout << "Total = " << total << ", Average = " << total / marks.size() << endl;
    cout << "Highest = " << *max_element(marks.begin(), marks.end()) << endl;

    auto it = find(marks.begin(), marks.end(), 85);
    if (it != marks.end()) cout << "85 found" << endl;

    // map: student name -> marks
    map<string, int> students;
    students["Asha"] = 85;
    students["Ravi"] = 72;
    students["Meera"] = 91;
    for (auto& p : students) cout << p.first << " : " << p.second << endl;
    return 0;
}
