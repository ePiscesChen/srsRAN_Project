#include <iostream>
#include <memory>
using namespace std;

class A {
public:
    virtual ~A() = default;
    virtual void F() = 0;
};

class B : public A {
public:
    B(){}
    ~B(){}
    void F() override{ cout<<0<<endl;}
};

class C {
public:
    C(){
        objA = make_unique<B>();
    }
    unique_ptr<A> objA;
    void print(){
        cout<<"c"<<endl;
    }
};
int main()
{
    C c;
    c.print();
    return 0;
}