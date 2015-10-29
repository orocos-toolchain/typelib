struct Base
{
    double field;
};

struct Derived : virtual public Base
{
    double other_field;
};
