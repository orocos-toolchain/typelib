#ifndef TEST_HEADER_DATA_TEMPLATES_H
#define TEST_HEADER_DATA_TEMPLATES_H

namespace templates {

    // just a testcase... is not detected, as there is not instantiation. even
    // if we name this type as "Opaque"!
    template<class T>
    class C1 {
      public:
        T A;
    };

    // in able to find the template named in the opaque, we need an acutal instantiation
    struct __gccxml_workaround_c1_double { templates::C1<double> foo; };

    // special trick: do an explicit template instantiation, and see if the
    // source_file_line property is correct -- which seems to be the case in the moment
    template <> class C1<float> {
      public:
        float B;
    };
    struct __gccxml_workaround_c1_float { templates::C1<float> foo; };

    namespace templateclasses {
    class D1 {
      public:
        // array as template parameter...? the actual array in C1 is
        // currently not detected, while the size-offset is correct.
        //
        // needs some more love... but using templates asks for pain.
        templates::C1<float[4]> m1;
        int m2;
    };
    }
}

#endif /*TEST_HEADER_DATA_TEMPLATES_H*/
