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
}


#endif /*TEST_HEADER_DATA_TYPEDEFS_H*/
