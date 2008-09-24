#ifndef TYPELIB_VALUE_OPS_HH
#define TYPELIB_VALUE_OPS_CC

#include <typelib/memory_layout.hh>
#include <typelib/value.hh>
#include <boost/tuple/tuple.hpp>

namespace Typelib
{
    /** This namespace includes the basic methods needed to implement complex
     * operations on Value objects (i.e. typed buffers)
     */
    namespace ValueOps
    {
        boost::tuple<size_t, MemoryLayout::const_iterator>
            dump(uint8_t* data, size_t in_offset,
                std::vector<uint8_t>& buffer,
                MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end);
        boost::tuple<size_t, size_t, MemoryLayout::const_iterator>
            load(uint8_t* data, size_t out_offset,
                std::vector<uint8_t> const& buffer, size_t in_offset,
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
    }

    void init(Value v);
    void destroy(Value v);
    void copy(Value dst, Value src);
    bool compare(Value dst, Value src);

    std::vector<uint8_t> dump(Value v);
    void dump(Value v, std::vector<uint8_t>& buffer);
    void dump(Value v, std::vector<uint8_t>& buffer, MemoryLayout const& ops);
    void load(Value v, std::vector<uint8_t> const& buffer);
    void load(Value v, std::vector<uint8_t> const& buffer, MemoryLayout const& ops);
}

#endif

