#ifndef TEST_HEADER_DATA_NEGATIVE_H
#define TEST_HEADER_DATA_NEGATIVE_H

namespace negative {

    // we don't want to have records with pointers
    struct S1 {
        int* pA;
    };
    struct S2 {
        const int* pA;
    };

    // no references
    struct S3 {
        int& pA;
    };
    struct S4 {
        const int& pA;
    };

    //  virtual and pure-virtual stuff
    class pureVirtual {
        virtual void F() = 0;
        int A;
    };
    class B : public pureVirtual {
        virtual void F() {};
    };
    class virtualClass {
        virtual void F();
        int A;
    };
    class C : public virtualClass {
        virtual void F();
        int B;
    };

    // anonymous structs and classes don't have a canonical name, so we cannot reference them later by name.
    struct {
        int A;
        int B;
    } anon_S1;
    typedef struct {
        int A;
        int B;
    } anon_S2;
    class {
        int A;
        int B;
    } anon_C1;
    typedef class {
        int A;
        int B;
    } anon_C2;
}


#endif /*TEST_HEADER_DATA_NEGATIVE_H*/
