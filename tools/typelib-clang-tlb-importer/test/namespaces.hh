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

// test "inline namespace": is done in "libc++" for example, used in OSX.
//
// see http://en.cppreference.com/w/cpp/language/namespace#Inline_namespaces
namespace standard {
    namespace __1 {
        // define the class inside some "temporary" namespace
        class StandardClass { public: int standardMember; };
    }
    // and then pull its content into the "standard" namespace
    using namespace __1;
}
// now I could create the following object (just for illustration, not needed
// for tlb-generation)
standard::StandardClass dut;
// the tlb-file will have entries like "standard::__1::StandardClass". this is
// what happens on OSX with their libc++: entries like "std::__1::vector" for
// example while we have an opaques named "std::vector" and don't match this.

#endif /*TEST_DATA_HEADER_NAMESPACES_H*/
