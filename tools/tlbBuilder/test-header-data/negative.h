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
}


#endif /*TEST_HEADER_DATA_NEGATIVE_H*/
