#ifndef TEST_HEADER_DATA_CONSTS_H
#define TEST_HEADER_DATA_CONSTS_H

namespace consts {

    typedef long unsigned int my_typedef;
    typedef long unsigned int my_other_typedef;

    typedef const long unsigned int my_const_typedef;
    typedef long unsigned const int my_other_const_typedef;

    struct C1 {
        my_const_typedef m1;
        my_other_const_typedef m2;
        my_other_typedef m3;
    };

    struct C2 {
        const int *m1;
        const double m2;
    };

}

#endif /*TEST_HEADER_DATA_CONSTS_H*/
