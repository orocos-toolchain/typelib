#ifndef TEST_HEADER_DATA_OPAQUES_H
#define TEST_HEADER_DATA_OPAQUES_H

namespace ns_opaques {

    // so that we have a database, no matter what
    struct cruft {
        int data[4];
    };

    // this class has a pointer, which should prevent it from beeing added to
    // the database. however, we added an entry into "opaques.opaques" where
    // the class is named, so it has to be be added.
    class C1 {
      public:
        float* a_float_pointer;
        struct cruft test;
    };
    // whereas this one has a reference in it and is not named as an opaque. it
    // should be ignored...
    class C2 {
      public:
        float& a_float_reference;
        struct cruft test;
    };

    // what works is providing "/opaques/typedef_of_C3" as opaque. filling the
    // metadata entries will be provided by the "typedef" path,
    // "opaque_is_typedef" for example will be set.
    template<typename T>
    class C3 {
        T* another_float_pointer;
    };
    typedef C3<float> typedef_of_C3;

    // what _not_ work easily is defining "/opaques/C4</float>" as opaque --
    // the compiler does not see the template. why?
    //
    // the opaque is actually never seen in the "opaque lookup phase" where the
    // code looks for namedDecl's...? what we have to look for are
    // "ClassTemplateDecl" with the name without template-parms
    template<typename T>
    class C4 {
        T* another_float_pointer;
    };
    // need an instantiation of the template-specialization which is named as
    // "opaque"
    C4<float> inst_C4;

    // the same, ust with two template-arguments -- for funz
    template<typename T, typename U>
    class C5 {
        T* another_float_pointer;
        U* just_more_members;
    };
    // ...instantiation
    C5<float,int> inst_C5;

}

#endif /*TEST_HEADER_DATA_OPAQUES_H*/
