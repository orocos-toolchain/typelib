#ifndef PACKING_CHECK_ARRAYS_TCC
#define PACKING_CHECK_ARRAYS_TCC

//
// We check that for all types B in podlists,
// struct { A B[10] } is packed as struct { A B }
//
 
namespace { namespace check_arrays {

    template<typename A, typename B>
    struct discover_arrays
    {
        typedef A  First;
        typedef B  Second;

        A first;
        B second[10];
    };
   
    // takes a type and a typelist, and builds
    // the typelist of packingof<type, it[10]> for each
    // type in SecondList
    template<class First, class SecondList>
    struct iterate_arrays
    {
        typedef typename fold
            < SecondList
            , list<>
            , push_front< _1, packingof< discover_arrays<First, _2> > >
            >::type type;
    };

    // Check the property for each POD in pods
    struct do_check_equality
    {
        typedef fold
            < iterate_arrays<char, podlist>::type
            , list<>
            , is_same_as_simple<_2>
            >::type type;
    };
}}

#endif

