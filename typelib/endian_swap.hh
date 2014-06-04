#ifndef __TYPELIB_SYSTEM_ENDIAN_HH
#define __TYPELIB_SYSTEM_ENDIAN_HH

#include <boost/integer.hpp>
#include <boost/detail/endian.hpp>

namespace Typelib
{
    /** This namespace holds various tools to convert between little and big endian 
     *
     * @ingroup system
     */
    namespace Endian
    {
	namespace Details {
	    template<int size> struct type_from_size
	    { typedef typename boost::uint_t<size>::least least; };
	    template<> struct type_from_size<64>
	    { typedef uint64_t least; };

	    template<int size, typename D>
	    void swap_helper(const D data, D& buffer);

	    template<> inline void swap_helper<1, uint8_t>(const uint8_t data, uint8_t& buffer)
	    { buffer = data; }
	    template<> inline void swap_helper<2, uint16_t>(const uint16_t data, uint16_t& buffer)
	    { buffer = ((data >> 8) & 0xFF) | ((data << 8) & 0xFF00); }
	    template<> inline void swap_helper<4, uint32_t>(const uint32_t data, uint32_t& buffer)
	    { buffer = ((data & 0xFF000000) >> 24) | ((data & 0x00FF0000) >> 8) | ((data & 0xFF) << 24) | ((data & 0xFF00) << 8); }
	    template<> inline void swap_helper<8, uint64_t>(const uint64_t data, uint64_t& buffer)
	    { 
		const uint32_t 
		      src_low (data & 0xFFFFFFFF)
		    , src_high(data >> 32);

		uint32_t dst_low, dst_high;
		swap_helper<4, uint32_t>( src_high, dst_low );
		swap_helper<4, uint32_t>( src_low, dst_high );
		
		buffer = static_cast<uint64_t>(dst_high) << 32 | dst_low;
	    }
	}

	/* Returns in \c buffer the value of \c data with endianness swapped */
	template<typename S>
	inline void swap(const S data, S& buffer)
	{ 
	    typedef typename Details::type_from_size<sizeof(S) * 8>::least T;
	    Details::swap_helper<sizeof(S), T> (reinterpret_cast<const T&>(data), reinterpret_cast<T&>(buffer)); 
	}

	/* Returns the value of \c data with endianness swapped */
	template<typename S>
	inline S swap(const S data)
	{ S ret;
	    swap(data, ret);
	    return ret;
	}

#if defined(BOOST_BIG_ENDIAN)
	/** Converts \c source, which is in native byte order, into big endian and 
	 * saves the result into \c dest */
	template<typename S>
	inline void to_big(const S source, S& dest) { dest = source; }
	/** Converts \c source, which is in native byte order, into big endian and 
	 * returns the result */
	template<typename S>
	inline S to_big(const S source) { return source; }
	/** Converts \c source, which is in native byte order, into little endian and 
	 * saves the result into \c dest */
	template<typename S>
	inline void to_little(const S source, S& dest) { swap<S>(source, dest); }
	/** Converts \c source, which is in native byte order, into little endian and 
	 * returns the result */
	template<typename S>
	inline S to_little(const S source) { return swap<S>(source); }
#elif defined(BOOST_LITTLE_ENDIAN)
	template<typename S>
	inline void to_big(const S source, S& dest) { swap<S>(source, dest); }
	template<typename S>
	inline S to_big(const S source) { return swap<S>(source); }
	template<typename S>
	inline void to_little(const S source, S& dest) { dest = source; }
	template<typename S>
	inline S to_little(const S source) { return source; }
#else
#       error "unknown endianness, neither BOOST_LITTLE_ENDIAN nor BOOST_BIG_ENDIAN are defined"
#endif

	/** Converts \c source, which is in network byte order, into native byte order and 
	 * saves the result into \c dest */
	template<typename S>
	inline void from_network(const S source, S& dest) { to_network(source, dest); }
	/** Converts \c source, which is in network byte order, into native byte order and
	 * returns the result */
	template<typename S>
	inline S from_network(const S source) { return to_network(source); }
	/** Converts \c source, which is in little endian, into native byte order and 
	 * saves the result into \c dest */
	template<typename S>
	inline void from_little(const S source, S& dest) { to_little(source, dest); }
	/** Converts \c source, which is in little endian, into native byte order and
	 * returns the result */
	template<typename S>
	inline S from_little(const S source) { return to_little(source); }
	/** Converts \c source, which is in big endian, into native byte order and 
	 * saves the result into \c dest */
	template<typename S>
	inline void from_big(const S source, S& dest) { to_big(source, dest); }
	/** Converts \c source, which is in big endian, into native byte order and
	 * returns the result */
	template<typename S>
	inline S from_big(const S source) { return to_big(source); }

	template<typename S>
	inline void to_network(const S source, S& dest) { return to_big(source, dest); }
	template<typename S>
	inline S to_network(const S source) { return to_big(source); }
    }
}

#endif

