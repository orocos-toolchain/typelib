#ifndef TEST_HEADER_DATA_OPAQUES_H
#define TEST_HEADER_DATA_OPAQUES_H

namespace opaques {

    // so that we have a database, no matter what
    struct cruft {
        int data[4];
    };

    // this class has a pointer, which should prevent it from beeing added to
    // the database. however, if we provide a opaques-file where the class is
    // named, it will still be added.
    class C1 {
        float* a_float_pointer;
        struct cruft test;
    };

    class C2 {
        float& a_float_reference;
        struct cruft test;
    };

}

#endif /*TEST_HEADER_DATA_OPAQUES_H*/
