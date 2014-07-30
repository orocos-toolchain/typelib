#ifndef TEST_HEADER_DATA_BITFIELD_H
#define TEST_HEADER_DATA_BITFIELD_H

// taken from http://en.cppreference.com/w/cpp/language/bit_field
//
// quote from there: "although this behavior is implementation-defined"... so
// take the examples here with a grain of salt concerning their importance...

namespace bitfield {

    struct S1 {
        // three-bit unsigned field,
        // allowed values are 0...7
        unsigned int b : 3;
    };

    struct S2 {
        // will usually occupy 2 bytes:
        // 3 bits: value of b1
        // 2 bits: unused
        // 6 bits: value of b2
        // 2 bits: value of b3
        // 3 bits: unused
        unsigned char b1 : 3, : 2, b2 : 6, b3 : 2;
    };

    struct S3 {
        // will usually occupy 2 bytes:
        // 3 bits: value of b1
        // 5 bits: unused
        // 6 bits: value of b2
        // 2 bits: value of b3
        unsigned char b1 : 3;
        unsigned char :0; // start a new byte
        unsigned char b2 : 6;
        unsigned char b3 : 2;
    };
}

#endif /*TEST_HEADER_DATA_BITFIELD_H*/
