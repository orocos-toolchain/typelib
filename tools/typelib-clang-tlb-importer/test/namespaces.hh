#ifndef TEST_DATA_HEADER_NAMESPACES_H
#define TEST_DATA_HEADER_NAMESPACES_H

// anonymous namespace
namespace {
    struct S1 {
        int A;
        double B;
        char C;
    };
}

// namespace in namespace
namespace N1 {
    namespace N2 {
        struct S1 {
            int A;
            double B;
            char C;
        };
    }
}

#endif /*TEST_DATA_HEADER_NAMESPACES_H*/
