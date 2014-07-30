#ifndef TEST_HEADER_DATA_STDVECTOR_H
#define TEST_HEADER_DATA_STDVECTOR_H

#include <vector>
#include <time.h>

// this file should document the behaviour of typelib concerning std::vector,
// which has a sonderrolle...

namespace stdvector {

    typedef std::vector<double> myvec;

    struct S1 {
        int a;
        std::vector<float> v1;
    };

    // vector of structs...
    struct S2 {
        std::vector<struct timespec> v1;
    };

    // typedef'ed vector:
    struct S3 {
        myvec v2;
    };

    // very fancy: array of vectors? who knows...
    class C1 {
        myvec v[2];
    };

}

#endif /*TEST_HEADER_DATA_STDVECTOR_H*/
