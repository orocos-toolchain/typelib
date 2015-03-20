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

// checking a c++11 related feature: "inline namespace", which is done in
// "libc++" for example, used in OSX. to test this you can call the
// importer directly and give it "-std=c++11" as additional argument
// 
// see http://en.cppreference.com/w/cpp/language/namespace#Inline_namespaces
#if __cplusplus > 199711L

namespace standard {
    namespace __1 {
        // define the class inside some "temporary" namespace
        class StandardClass { public: int standardMember; };
    }
    // and then pull its content into the "standard" namespace
    using namespace __1;
}
// now I can create the following object:
standard::StandardClass dut;
// the tlb-file will have entries like "standard::__1::StandardClass". this is
// what happens on OSX with their libc++: entries like "std::__1::vector" for
// example while we have an opaques named "std::vector" and don't match this.

#endif

#endif /*TEST_DATA_HEADER_NAMESPACES_H*/
