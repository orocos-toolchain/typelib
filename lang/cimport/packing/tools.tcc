#ifndef PACKING_CHECK_TOOLS_TCC
#define PACKING_CHECK_TOOLS_TCC

#include <boost/mpl/fold.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/list.hpp>
#include <boost/static_assert.hpp>

namespace
{
    using namespace boost::mpl;

    template<typename A, typename B>
    struct discover
    {
        typedef A First;
        typedef B Second;

        A first;
        B second;
    };

    /** Given a discover type (one of discover and discover_arrays)
     * returns the offset of second in
     *
     *   struct { First first; Second second; };
     *
     */
    template<class Discover>
    struct packingof
    {
        typedef typename Discover::First    First;
        typedef typename Discover::Second   Second;

        static const int packing = offsetof(Discover, second);
    };

    // takes a type and a typelist, and builds
    // the typelist of packingof<type, it> for each
    // type in SecondList
    template<class First, class SecondList>
    struct iterate_second
    {
        typedef typename fold
            < SecondList
            , list<>
            , push_front< _1, packingof< discover<First, _2> > >
            >::type type;
    };

    typedef list<char, short, int, long, float, double, char*> podlist;

    template<class Packing>
    struct is_same_as_simple
    {
        BOOST_STATIC_ASSERT(( 
            Packing::packing == 
            packingof
                < discover
                    < typename Packing::First
                    , typename Packing::Second
                    >
                >::packing
            ));
    };
}


#endif

