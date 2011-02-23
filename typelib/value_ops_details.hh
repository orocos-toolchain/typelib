#ifndef TYPELIB_VALUE_OPS_DETAILS_HH
#define TYPELIB_VALUE_OPS_DETAILS_HH

#include <typelib/memory_layout.hh>
#include <typelib/value.hh>
#include <iosfwd>

#include <boost/tuple/tuple.hpp>

namespace Typelib
{
    /** This namespace includes the basic methods needed to implement complex
     * operations on Value objects (i.e. typed buffers)
     */
    namespace ValueOps
    {
        boost::tuple<size_t, MemoryLayout::const_iterator>
            dump(uint8_t const* data, size_t in_offset,
                OutputStream& stream, MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end);

        boost::tuple<size_t, MemoryLayout::const_iterator>
            load(uint8_t* data, size_t out_offset,
                InputStream& stream,
                MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end);

        boost::tuple<bool, uint8_t*, uint8_t*, MemoryLayout::const_iterator>
            compare(uint8_t* out_buffer, uint8_t* in_buffer,
                    MemoryLayout::const_iterator it,
                    MemoryLayout::const_iterator end);
        boost::tuple<uint8_t*, uint8_t*, MemoryLayout::const_iterator>
            copy(uint8_t* out_buffer, uint8_t* in_buffer,
                    MemoryLayout::const_iterator it,
                    MemoryLayout::const_iterator end);
        boost::tuple<uint8_t*, MemoryLayout::const_iterator>
            destroy(uint8_t* buffer,
                    MemoryLayout::const_iterator it,
                    MemoryLayout::const_iterator end);
        boost::tuple<uint8_t*, MemoryLayout::const_iterator>
            init(uint8_t* buffer,
                    MemoryLayout::const_iterator it,
                    MemoryLayout::const_iterator end);
        boost::tuple<uint8_t*, MemoryLayout::const_iterator>
            zero(uint8_t* buffer,
                    MemoryLayout::const_iterator it,
                    MemoryLayout::const_iterator end);
        boost::tuple<size_t, MemoryLayout::const_iterator>
            getDumpSize(uint8_t* buffer,
                    MemoryLayout::const_iterator it,
                    MemoryLayout::const_iterator end);
    }
}

#endif

