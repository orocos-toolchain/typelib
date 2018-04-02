#ifndef TEST_HEADER_DATA_ARRAYS_H
#define TEST_HEADER_DATA_ARRAYS_H

#include <time.h>

#define MY_INT unsigned long long

namespace arrays {

    /** inspired from "laser.h"
     *
     * but, as a courtesy, with a BlockCommandComment
     */
    struct D1 {
        int sec;
        unsigned int usec;
        double ranges[256];
        MY_INT a;
    };

    // not detected because?
    double A1[3];

    // two-dimensional array -- not detected as it is not record?
    double A2[3][5];
    // two-dimensional fields in records...
    class U1 {
      public:
        int unsupported_array_shoul_not_crash[1][2];
    };

    // array of records -- also not detected
    struct timespec A3[2];

    /// more text, formatted to be able to be extracted as a
    /// "clang::comments::FullComment".
    ///
    /// blalba. more text. the same wrapped inside a container -- detected and exported
    class C1 {
      public:
        struct timespec A3[2];
        int m1;
    };
}

#endif /*TEST_HEADER_DATA_ARRAYS_H*/
