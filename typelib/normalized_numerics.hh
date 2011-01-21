#ifndef NORMALIZED_NUMERICS_HH
#define NORMALIZED_NUMERICS_HH

#include <boost/static_assert.hpp>

namespace Typelib {
    namespace details {
        template<int Bits> class sint_t;
        template<> struct sint_t<7>  { typedef int8_t type; };
        template<> struct sint_t<15> { typedef int16_t type; };
        template<> struct sint_t<31> { typedef int32_t type; };
        template<> struct sint_t<63> { typedef int64_t type; };

        template<int Bits> class uint_t;
        template<> struct uint_t<8>  { typedef uint8_t type; };
        template<> struct uint_t<16> { typedef uint16_t type; };
        template<> struct uint_t<32> { typedef uint32_t type; };
        template<> struct uint_t<64> { typedef uint64_t type; };
    }

    /** This template converts base C types (long, int, ...) in their normalized form (uin8_t, int16_t, ...) 
     * For consistency, it is also specialized for float and double */
    template<typename T>
    struct normalized_numeric_type 
    {
        typedef std::numeric_limits<T>  limits;
        BOOST_STATIC_ASSERT(( limits::is_integer )); // will specialize for float and double

        typedef typename boost::mpl::if_
            < boost::mpl::bool_<std::numeric_limits<T>::is_signed>
            , details::sint_t< limits::digits >
            , details::uint_t< limits::digits >
            >::type                             getter;
        typedef typename getter::type           type;
    };

    template<> struct normalized_numeric_type<float>  { typedef float	type; };
    template<> struct normalized_numeric_type<double> { typedef double	type; };
}

#endif



