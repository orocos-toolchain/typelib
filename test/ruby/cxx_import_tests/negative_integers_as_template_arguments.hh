template<int value>
class Test
{
public:
    double field;
    Test()
        : field(value) {}
};

struct Instanciator : public Test<-2> {};
