// Experiment 7: Generic Stack and Queue using templates
#include <iostream>
#include <string>
using namespace std;

template <class T>
class Stack {
    T items[10];
    int top;
public:
    Stack() { top = -1; }
    void push(T x) {
        if (top == 9) cout << "Stack full" << endl;
        else items[++top] = x;
    }
    T pop() { return items[top--]; }
    bool empty() { return top == -1; }
};

template <class T>
class Queue {
    T items[10];
    int front, rear;
public:
    Queue() { front = 0; rear = 0; }
    void enqueue(T x) {
        if (rear == 10) cout << "Queue full" << endl;
        else items[rear++] = x;
    }
    T dequeue() { return items[front++]; }
    bool empty() { return front == rear; }
};

int main() {
    Stack<int> s1;
    s1.push(1); s1.push(2); s1.push(3);
    cout << "Int stack: ";
    while (!s1.empty()) cout << s1.pop() << " ";
    cout << endl;

    Stack<string> s2;
    s2.push("one"); s2.push("two");
    cout << "String stack: ";
    while (!s2.empty()) cout << s2.pop() << " ";
    cout << endl;

    Queue<float> q;
    q.enqueue(1.5f); q.enqueue(2.5f); q.enqueue(3.5f);
    cout << "Float queue: ";
    while (!q.empty()) cout << q.dequeue() << " ";
    cout << endl;
    return 0;
}
