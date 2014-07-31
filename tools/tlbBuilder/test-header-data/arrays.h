#ifndef TEST_HEADER_DATA_ARRAYS_H
#define TEST_HEADER_DATA_ARRAYS_H

#include <time.h>

namespace arrays {

    // taken from "laser.h"
    struct D1 {
        int sec;
        unsigned int usec;
        double ranges[256];
    };

    // simple array
    double A1[3];

    // two-dimensional array
    double A2[3][5];

    // array of records
    struct timespec A3[2];

}

#endif /*TEST_HEADER_DATA_ARRAYS_H*/
