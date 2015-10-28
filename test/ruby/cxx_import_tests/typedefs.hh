#ifndef TEST_HEADER_DATA_TYPEDEFS_H
#define TEST_HEADER_DATA_TYPEDEFS_H

#include <stdint.h>

// what should be done here:
//
// - we want to be able to use a typedef'ed type (intTypedef, intTypedefTypedef
//   and cClassTypedef) inside a record
//

namespace ns {

    // a class which is typedefed
    class aClass { public: int A; };
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
    // this is given as the actual opaque, and so we expect that the 'struct
    // S1' is added to the database
    typedef struct S1 myTestTypedef;

    // same here, only that S2 itself (and not a typedef on it) is an opaque
    // defined in the "typedefs.opaques"
    struct S2 {
        float *a;
    };

    // add another case without instantiation: we specified an opaque of
    // '/ns/C2</float>'in the file "typedefs.opaques" which names an
    // uninstantiated specialization of the following template-class. the
    // actual <float>-template is never seen by the compiler and hence no
    // "source_file_line" is added to the <opaque> entry of the database.
    template<class T>
    class C2 {
        T A;
    };

    // this wants to test how we proceed with typedefs that are declared, but
    // don't have actually a place or location where their content is defined.
    typedef struct not_defined testTypedef;

}

#endif /*TEST_HEADER_DATA_TYPEDEFS_H*/
