struct Base
{
    double first;
};

struct Derived : private Base
{
    double second;
};
