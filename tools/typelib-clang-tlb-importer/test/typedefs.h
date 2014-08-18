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
    // which is (by itself) an invalid typelib-datatype. but the compound has
    // as a typedef which is named in the "typedefs.opaque" file in the
    // test-dir.
    struct S1 {
        int a;
        float *c; // this would invalidate this struct if it would not be named
                  // as a opaque
    };
    typedef struct S1 myTestTypedef;

}


#endif /*TEST_HEADER_DATA_TYPEDEFS_H*/
