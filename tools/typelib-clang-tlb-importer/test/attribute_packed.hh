#ifndef TEST_HEADER_DATA_ATTRIBUTE_PACKED_H
#define TEST_HEADER_DATA_ATTRIBUTE_PACKED_H

#include <stdint.h>

// when using "packed" the messages get efficient, the compiler is not allowed
// to leave "holes" in records for better architecture-specific memory
// accesses.

namespace attribute_packed {

    // this should have a site of 20bytes
    struct S1 {
        uint8_t a;
        int b;
        int64_t c;
        int8_t d;
        float e;
        uint16_t f;
    } __attribute__((packed));

    // same member structure, but a class...
    class C1 {
      public:
        uint8_t a;
        int b;
        int64_t c;
        int8_t d;
        float e;
        uint16_t f;
    } __attribute__((packed));

}

#endif /*TEST_HEADER_DATA_ATTRIBUTE_PACKED_H*/
