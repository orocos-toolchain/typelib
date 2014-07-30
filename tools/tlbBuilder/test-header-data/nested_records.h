#ifndef TEST_HEADER_DATA_NESTED_RECORDS_H
#define TEST_HEADER_DATA_NESTED_RECORDS_H

#include <time.h>

namespace nested_records {

    // this is the struct later included
    struct S1 {
        int a;
        unsigned int b;
    };

    // here we pull the previously defined struct
    struct S2 {
        int m;
        S1 n;
    };

    // now pulling a struct from a system header
    struct S3 {
        struct timespec t;
    };

}

#endif /*TEST_HEADER_DATA_NESTED_RECORDS_H*/
