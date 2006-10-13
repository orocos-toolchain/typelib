#ifndef TYPELIB_ENDIANSWAP_HH
#define TYPELIB_ENDIANSWAP_HH

#include <typelib/value.hh>
#include <utilmm/system/endian.hh>

namespace Typelib
{
    /* This visitor swaps the endianness of the given value in-place */
    class EndianSwapVisitor : public ValueVisitor
    {
    protected:
        bool visit_ (int8_t  & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (uint8_t & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (int16_t & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (uint16_t& value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (int32_t & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (uint32_t& value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (int64_t & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (uint64_t& value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (float   & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (double  & value) { utilmm::endian_swap(value, value); return true; }
        bool visit_ (Enum::integral_type& v, Enum const& e) { utilmm::endian_swap(v, v); return true; }
    };

    /** Swaps the endianness of +v+ by using a EndianSwapVisitor */
    void endian_swap(Value v)
    {
	EndianSwapVisitor swapper;
	swapper.apply(v);
    }
}

#endif

