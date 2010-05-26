#ifndef TYPELIB_VALUE_OPS_HH
#define TYPELIB_VALUE_OPS_HH

#include <typelib/memory_layout.hh>
#include <typelib/value.hh>
#include <boost/tuple/tuple.hpp>
#include <iosfwd>
#include <stdio.h>

namespace Typelib
{
    /** This namespace includes the basic methods needed to implement complex
     * operations on Value objects (i.e. typed buffers)
     */
    namespace ValueOps
    {
        struct OutputStream
        {
            virtual void write(uint8_t const* data, size_t size) = 0;
        };
        boost::tuple<size_t, MemoryLayout::const_iterator>
            dump(uint8_t const* data, size_t in_offset,
                OutputStream& stream, MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end);

        struct InputStream
        {
            virtual void read(uint8_t* data, size_t size) = 0;
        };
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

    void init(Value v);
    void init(Value v, MemoryLayout const& ops);
    void init(uint8_t* data, MemoryLayout const& ops);

    void zero(Value v);
    void zero(Value v, MemoryLayout const& ops);
    void zero(uint8_t* data, MemoryLayout const& ops);

    void destroy(Value v);
    void destroy(Value v, MemoryLayout const& ops);
    void destroy(uint8_t* data, MemoryLayout const& ops);

    void copy(Value dst, Value src);
    void copy(void* dst, void* src, Type const& type);

    bool compare(Value dst, Value src);
    bool compare(void* dst, void* src, Type const& type);

    void display(std::ostream& io,
            MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end);

    std::vector<uint8_t> dump(Value v);
    void dump(Value v, std::vector<uint8_t>& buffer);
    void dump(Value v, std::vector<uint8_t>& buffer, MemoryLayout const& ops);
    void dump(uint8_t const* v, std::vector<uint8_t>& buffer, MemoryLayout const& ops);

    void dump(Value v, std::ostream& stream);
    void dump(Value v, std::ostream& stream, MemoryLayout const& ops);
    void dump(uint8_t const* v, std::ostream& stream, MemoryLayout const& ops);

    void dump(Value v, int fd);
    void dump(Value v, int fd, MemoryLayout const& ops);
    void dump(uint8_t const* v, int fd, MemoryLayout const& ops);

    void dump(Value v, FILE* fd);
    void dump(Value v, FILE* fd, MemoryLayout const& ops);
    void dump(uint8_t const* v, FILE* fd, MemoryLayout const& ops);

    size_t getDumpSize(Value v);
    size_t getDumpSize(Value v, MemoryLayout const& ops);
    size_t getDumpSize(uint8_t const* v, MemoryLayout const& ops);

    void load(Value v, std::vector<uint8_t> const& buffer);
    void load(Value v, std::vector<uint8_t> const& buffer, MemoryLayout const& ops);
    void load(uint8_t* v, std::vector<uint8_t> const& buffer, MemoryLayout const& ops);
}

#endif

