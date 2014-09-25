#ifndef TEST_HEADER_DATA_TYPEDEFS_H
#define TEST_HEADER_DATA_TYPEDEFS_H

#include <stdint.h>

// what should be done here:
//
// - we want to be able to use a typedef'ed type (intTypedef, intTypedefTypedef and cClassTypedef)
//   inside a record
//

namespace ns {

    // a class which is typedefed
    class aClass { int A; };
    typedef aClass aClassTypedef;

    // a simple typedef
    typedef int intTypedef;
    typedef int anotherIntTypedef;
    // a two-level typedef
    typedef intTypedef intTypedefTypedef;

    // the struct-struct
    struct test {
        uint8_t C;
        aClassTypedef A;
        intTypedef B;
        intTypedefTypedef D;
    };

    // testing the opaque-system using typedefs. therefore we need a "compound"
    // which is (by itself) an invalid typelib-datatype because it contains a
    // pointer. but the compound has as a typedef which is named in the
    // "typedefs.opaque" file in the test-dir.
    struct S1 {
        int a;
        float *c; // this would invalidate this struct if it would not be named
                  // as a opaque
    };
    // this is given as the actual opaque
    typedef struct S1 myTestTypedef;


    // same here, only that S2 itself is an opaque defined in the database
    struct S2 {
        float *a;
    };

    // this wants to test how we proceed with typedefs that are declared, but
    // don't have actually a place or location where their content is defined
    typedef struct not_defined testTypedef;


    // but we add a second case without instantiation, where we have an opaque
    template<class T>
    class C2 {
        T A;
    };
}


#endif /*TEST_HEADER_DATA_TYPEDEFS_H*/
