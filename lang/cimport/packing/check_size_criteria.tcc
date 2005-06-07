#ifndef PACKING_CHECK_SIZE_CRITERIA_TCC
#define PACKING_CHECK_SIZE_CRITERIA_TCC

//
// Check that struct { A B } is packed as struct { A C } if sizeof(B) == sizeof(C)
// 
 
namespace { namespace check_size_criteria {
    template<bool size_equals, typename Packing, typename C>
    struct check_packing3;
    template<typename Packing, typename C>
    struct check_packing3<false, Packing, C> 
    { typedef Packing type; };
    // Checks if struct { A B } is packed as struct { A C }
    template<typename Packing, typename C>
    struct check_packing3<true, Packing, C>
    { 
        BOOST_STATIC_ASSERT((
            Packing::packing
            == packingof
                < discover
                    < typename Packing::First
                    , C 
                    > 
                >::packing

            ));

        typedef Packing type;
    };

    template<typename Packing, typename C>
    struct check_packing3_helper
    { 
        typedef typename check_packing3
                < sizeof(typename Packing::Second) == sizeof(C)
                , Packing
                , C 
                >::type type; 
    };

    template<typename A, typename Base>
    struct do_check_packing3
    {
        typedef typename fold
            < podlist
            , packingof< discover<A, Base> >
            , check_packing3_helper<_1, _2>
            >::type    type;
    };

    struct forpod_check_packing3
    {
        typedef fold
            < podlist
            , list<>
            , push_front< _1, do_check_packing3<char, _2> >
            >::type type;
    };

}}

#endif


