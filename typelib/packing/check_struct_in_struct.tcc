#ifndef PACKING_CHECK_STRUCT_IN_STRUCT_HH
#define PACKING_CHECK_STRUCT_IN_STRUCT_HH

//
// Check that struct { A struct { B } } is packed as struct { A B } is
//

namespace { namespace check_struct_in_struct {
   
    template<typename A, typename B> 
    struct discover_struct 
    { 
        typedef A First;
        typedef B Second;

        A first;
        struct { B inner; } second;
    };

    // takes a type and a typelist, and builds
    // the typelist of packingof<type,  struct { it } > for each
    // type in SecondList
    template<class First, class SecondList>
    struct iterate_struct
    {
        typedef typename fold
            < SecondList
            , list<>
            , push_front< _1, packingof< discover_struct<First, _2> > >
            >::type type;
    };

    // Applies check_struct_packing for <first, struct > for each struct in podstructs
    // It is equivalent as applying it to <first, simple_struct<pod> > for each pod in podlist
    struct do_check_struct_packing
    {
        typedef fold
            < iterate_struct<char, podlist>::type
            , list<>
            , is_same_as_simple<_2>
            >::type type;
    };
}}

#endif

