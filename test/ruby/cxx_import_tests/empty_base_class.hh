#ifndef TEST_HEADER_EMPTY_BASE_CLASS_HH
#define TEST_HEADER_EMPTY_BASE_CLASS_HH

struct Base {};

struct PublicDerived : public Base {
    int field;
};

struct PrivateDerived : private Base {
    int field;
};

struct VirtualDerived : public virtual Base {
    int field;
};

struct BaseWithVirtual {
    virtual void something() { };
};

struct DerivedWithVirtual : public BaseWithVirtual {
    int field;
};

#endif
