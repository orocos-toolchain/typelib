#include <typelib/value_ops.hh>
#include <string.h>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "value_ops_details.hh"
#include <cstdio>

using namespace Typelib;
using namespace boost;
using namespace std;

void Typelib::display(std::ostream& io, MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end)
{
    io << "displaying memory layout of size " << end - begin << "\n";
    for (MemoryLayout::const_iterator it = begin; it != end; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                io << "MEMCPY " << size << "\n";
                break;
            }
            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                io << "ARRAY " << element_count << "\n";
                break;
            }
            case MemLayout::FLAG_SKIP:
            {
                size_t size  = *(++it);
                io << "SKIP " << size << "\n";
                break;
            }
            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                io << "CONTAINER " << type->getName() << "\n";
                break;
            }
            case MemLayout::FLAG_END:
            {
                io << "END\n";
                break;
            }
            default:
            {
                io << "in display(): unrecognized marshalling bytecode " << *it << " at " << (it - begin) << "\n";
                throw UnknownLayoutBytecode();
            }
        }
    }

}


tuple<size_t, MemoryLayout::const_iterator> ValueOps::dump(
        boost::uint8_t const* data, size_t in_offset,
        OutputStream& stream, MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end)
{
    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                stream.write(data + in_offset, size);
                in_offset += size;
                break;
            }
            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;
                if (element_count == 0)
                    it = MemLayout::skip_block(element_it, end);
                else
                {
                    for (size_t i = 0; i < element_count; ++i)
                        tie(in_offset, it) = dump(data, in_offset, stream, element_it, end);
                }

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode: array does not end with FLAG_END");

                break;
            }
            case MemLayout::FLAG_SKIP:
            {
                size_t size  = *(++it);
                in_offset += size;
                break;
            }
            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                // First, dump the element count into the stream
                void const* container_ptr  = data + in_offset;
                in_offset += type->getSize();

                boost::uint64_t element_count = type->getElementCount(container_ptr);
                stream.write(reinterpret_cast< boost::uint8_t* >(&element_count), sizeof(element_count));

                if (element_count == 0)
                    it = MemLayout::skip_block(++it, end);
                else
                    it = type->dump(container_ptr, element_count, stream, ++it, end);

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in bytecode while dumping: container does not end with FLAG_END");
                break;
            }
            default:
                throw UnknownLayoutBytecode();
        }
    }

    return make_tuple(in_offset, it);
}

tuple<size_t, MemoryLayout::const_iterator> ValueOps::load(
        boost::uint8_t* data, size_t out_offset,
        InputStream& stream, MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end)
{
    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                stream.read(data + out_offset, size);
                out_offset += size;
                break;
            }
            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;

                if (element_count == 0)
                    it = MemLayout::skip_block(element_it, end);
                else
                {
                    for (size_t i = 0; i < element_count; ++i)
                        tie(out_offset, it) =
                            load(data, out_offset, stream, element_it, end);
                }

                if (*it != MemLayout::FLAG_END)
                    throw std::runtime_error("bytecode error in load(): array does not end with FLAG_END");

                break;
            }
            case MemLayout::FLAG_SKIP:
            {
                size_t size  = *(++it);
                out_offset += size;
                break;
            }
            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                void* container_ptr  = data + out_offset;
                out_offset += type->getSize();

                boost::uint64_t element_count;
                stream.read(reinterpret_cast< boost::uint8_t* >(&element_count), sizeof( boost::uint64_t ));
                if (element_count == 0)
                    it = MemLayout::skip_block(++it, end);
                else
                {
                    it = type->load(container_ptr, element_count, stream, ++it, end);
                }

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("bytecode error in load(): container does not end with FLAG_END");
                break;
            }
            default:
                throw UnknownLayoutBytecode();
        }
    }

    return make_tuple(out_offset, it);
}


void Typelib::init(Value v)
{
    MemoryLayout ops = layout_of(v.getType(), true);
    init(v, ops);
}

void Typelib::init(Value v, MemoryLayout const& ops)
{
    boost::uint8_t* buffer = reinterpret_cast< boost::uint8_t* >(v.getData());
    init(buffer, ops);
}

void Typelib::init(boost::uint8_t* data, MemoryLayout const& ops)
{
    ValueOps::init(data, ops.begin(), ops.end());
}

void Typelib::zero(Value v)
{
    MemoryLayout ops = layout_of(v.getType(), true);
    zero(v, ops);
}

void Typelib::zero(Value v, MemoryLayout const& ops)
{
    boost::uint8_t* buffer = reinterpret_cast< boost::uint8_t* >(v.getData());
    zero(buffer, ops);
}

void Typelib::zero(boost::uint8_t* data, MemoryLayout const& ops)
{
    ValueOps::zero(data, ops.begin(), ops.end());
}

void Typelib::destroy(Value v)
{
    boost::uint8_t* buffer = reinterpret_cast< boost::uint8_t* >(v.getData());
    MemoryLayout ops = layout_of(v.getType(), true);
    destroy(buffer, ops);
}

void Typelib::destroy(Value v, MemoryLayout const& ops)
{
    destroy(reinterpret_cast< boost::uint8_t* >(v.getData()), ops);
}

void Typelib::destroy(boost::uint8_t* data, MemoryLayout const& ops)
{
    ValueOps::destroy(data, ops.begin(), ops.end());
}

void Typelib::copy(Value dst, Value src)
{
    if (dst.getType() != src.getType())
        throw std::runtime_error("requested copy with incompatible types");

    copy(dst.getData(), src.getData(), src.getType());
}

void Typelib::copy(void* dst, void* src, Type const& type)
{
    if (dst == src)
    {
        // same object, nothing to do
        return;
    }

    boost::uint8_t* out_buffer = reinterpret_cast< boost::uint8_t* >(dst);
    boost::uint8_t* in_buffer  = reinterpret_cast< boost::uint8_t* >(src);
    MemoryLayout ops = layout_of(type);
    ValueOps::copy(out_buffer, in_buffer, ops.begin(), ops.end());
}

bool Typelib::compare(Value dst, Value src)
{
    if (!dst.getType().canCastTo(src.getType()))
        return false;

    return compare(dst.getData(), src.getData(), dst.getType());
}

bool Typelib::compare(void* dst, void* src, Type const& type)
{
    boost::uint8_t* out_buffer = reinterpret_cast< boost::uint8_t* >(dst);
    boost::uint8_t* in_buffer  = reinterpret_cast< boost::uint8_t* >(src);

    MemoryLayout ret;
    MemLayout::Visitor visitor(ret);
    visitor.apply(type, false);

    bool is_equal;
    tie(is_equal, tuples::ignore, tuples::ignore, tuples::ignore) =
        ValueOps::compare(out_buffer, in_buffer, ret.begin(), ret.end());
    return is_equal;
}

tuple< boost::uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::zero(boost::uint8_t* buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            case MemLayout::FLAG_SKIP:
            {
                size_t size = *(++it);
                memset(buffer, 0, size);
                buffer += size;
                break;
            }

            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;
                for (size_t i = 0; i < element_count; ++i)
                    tie(buffer, it) = zero(buffer, element_it, end);

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");
                break;
            }

            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                type->clear(buffer);
                it = MemLayout::skip_block(++it, end);
                buffer += type->getSize();
                break;
            }

            default:
                throw std::runtime_error("in zero(): unrecognized marshalling bytecode " + boost::lexical_cast<std::string>(*it));
        }
    }

    return make_tuple(buffer, it);
}
tuple< boost::uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::init(uint8_t* buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            case MemLayout::FLAG_SKIP:
            {
                buffer += *(++it);
                break;
            }

            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;
                for (size_t i = 0; i < element_count; ++i)
                    tie(buffer, it) = init(buffer, element_it, end);

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");
                break;
            }

            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                type->init(buffer);
                it = MemLayout::skip_block(++it, end);
                buffer += type->getSize();
                break;
            }

            default:
                throw std::runtime_error("in init(): unrecognized marshalling bytecode " + boost::lexical_cast<std::string>(*it));
        }
    }

    return make_tuple(buffer, it);
}

tuple< boost::uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::destroy(boost::uint8_t* buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{
    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            case MemLayout::FLAG_SKIP:
            {
                buffer += *(++it);
                break;
            }

            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;
                for (size_t i = 0; i < element_count; ++i)
                    tie(buffer, it) = destroy(buffer, element_it, end);

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");
                break;
            }

            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                type->destroy(buffer);
                it = MemLayout::skip_block(it, end);
                buffer += type->getSize();
                break;
            }

            default:
                throw std::runtime_error("in destroy(): unrecognized marshalling bytecode " + boost::lexical_cast<std::string>(*it));
        }
    }

    return make_tuple(buffer, it);
}

tuple< boost::uint8_t*, boost::uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::copy(boost::uint8_t* out_buffer, boost::uint8_t* in_buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                memcpy(out_buffer, in_buffer, size);
                out_buffer += size;
                in_buffer  += size;
                break;
            }
            case MemLayout::FLAG_SKIP:
            {
                size_t size = *(++it);
                out_buffer += size;
                in_buffer  += size;
                break;
            }
            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;
                for (size_t i = 0; i < element_count; ++i)
                    tie(out_buffer, in_buffer, it) =
                        copy(out_buffer, in_buffer, element_it, end);

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");
                break;
            }
            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                type->copy(out_buffer, in_buffer);
                it = MemLayout::skip_block(it, end);
                out_buffer += type->getSize();
                in_buffer  += type->getSize();
                break;
            }
            default:
                throw std::runtime_error("in copy(): unrecognized marshalling bytecode " + boost::lexical_cast<std::string>(*it));
        }
    }

    return make_tuple(out_buffer, in_buffer, it);
}

tuple<bool, boost::uint8_t*, boost::uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::compare(boost::uint8_t* out_buffer, boost::uint8_t* in_buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                if (memcmp(out_buffer, in_buffer, size))
                    return make_tuple(false, (boost::uint8_t*)0, (boost::uint8_t*)0, end);

                out_buffer += size;
                in_buffer  += size;
                break;
            }
            case MemLayout::FLAG_SKIP:
            {
                size_t size = *(++it);
                out_buffer += size;
                in_buffer  += size;
                break;
            }
            case MemLayout::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MemoryLayout::const_iterator element_it = ++it;
                for (size_t i = 0; i < element_count; ++i)
                {
                    bool is_equal;
                    tie(is_equal, out_buffer, in_buffer, it) =
                        compare(out_buffer, in_buffer, element_it, end);
                    if (!is_equal)
                        return make_tuple(false, (boost::uint8_t*)0, (boost::uint8_t*)0, end);
                }

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");
                break;
            }
            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                if (!type->compare(out_buffer, in_buffer))
                    return make_tuple(false, (boost::uint8_t*)0, (boost::uint8_t*)0, end);

                it = MemLayout::skip_block(it, end);
                out_buffer += type->getSize();
                in_buffer  += type->getSize();
                break;
            }
            default:
                throw std::runtime_error("in compare(): unrecognized marshalling bytecode " + boost::lexical_cast<std::string>(*it));
        }
    }
    return make_tuple(true, out_buffer, in_buffer, it);
}



std::vector< boost::uint8_t > Typelib::dump(Value v)
{
    std::vector< boost::uint8_t > buffer;
    dump(v, buffer);
    return buffer;
}

/*--------------------------------------------------
 * Dump support to std::vector< boost::uint8_t >
 */
struct VectorOutputStream : public OutputStream
{
    std::vector< boost::uint8_t >& buffer;
    VectorOutputStream(std::vector< boost::uint8_t >& buffer)
        : buffer(buffer) {}

    void write(boost::uint8_t const* data, size_t size)
    {
        size_t out_index = buffer.size(); buffer.resize(out_index + size);
        memcpy(&buffer[out_index], data, size);
    }
};
void Typelib::dump(Value v, std::vector< boost::uint8_t >& buffer)
{
    VectorOutputStream stream(buffer);
    return dump(v, stream);
}
void Typelib::dump(Value v, std::vector< boost::uint8_t >& buffer, MemoryLayout const& ops)
{
    VectorOutputStream stream(buffer);
    return dump(v, stream, ops);
}
void Typelib::dump( boost::uint8_t const* v, std::vector< boost::uint8_t >& buffer, MemoryLayout const& ops)
{
    VectorOutputStream stream(buffer);
    return dump(v, stream, ops);
}

/*--------------------------------------------------
 * Dump support to std::ostream
 */
struct OstreamOutputStream : public OutputStream
{
    std::ostream& stream;
    OstreamOutputStream(std::ostream& stream)
        : stream(stream) {}

    void write(boost::uint8_t const* data, size_t size)
    {
        stream.write(reinterpret_cast<char const*>(data), size);
    }
};
void Typelib::dump(Value v, std::ostream& ostream)
{
    OstreamOutputStream stream(ostream);
    return dump(v, stream);
}
void Typelib::dump(Value v, std::ostream& ostream, MemoryLayout const& ops)
{
    OstreamOutputStream stream(ostream);
    return dump(v, stream, ops);
}
void Typelib::dump(boost::uint8_t const* v, std::ostream& ostream, MemoryLayout const& ops)
{
    OstreamOutputStream stream(ostream);
    return dump(v, stream, ops);
}


/*--------------------------------------------------
 * Dump support to std::ostream
 */
struct FDOutputStream : public OutputStream
{
    int fd;
    FDOutputStream(int fd)
        : fd(fd) {}

    void write(boost::uint8_t const* data, size_t size)
    {
        ::write(fd, data, size);
    }
};
void Typelib::dump(Value v, int fd)
{
    FDOutputStream stream(fd);
    return dump(v, stream);
}
void Typelib::dump(Value v, int fd, MemoryLayout const& ops)
{
    FDOutputStream stream(fd);
    return dump(v, stream, ops);
}
void Typelib::dump(boost::uint8_t const* v, int fd, MemoryLayout const& ops)
{
    FDOutputStream stream(fd);
    return dump(v, stream, ops);
}


/*--------------------------------------------------
 * Dump support to FILE
 */
struct FileOutputStream : public OutputStream
{
    FILE* fd;
    FileOutputStream(FILE* fd)
        : fd(fd) {}

    void write(boost::uint8_t const* data, size_t size)
    {
        ::fwrite(data, size, 1, fd);
    }
};
void Typelib::dump(Value v, FILE* fd)
{
    FileOutputStream stream(fd);
    return dump(v, stream);
}
void Typelib::dump(Value v, FILE* fd, MemoryLayout const& ops)
{
    FileOutputStream stream(fd);
    return dump(v, stream, ops);
}
void Typelib::dump(boost::uint8_t const* v, FILE* fd, MemoryLayout const& ops)
{
    FileOutputStream stream(fd);
    return dump(v, stream, ops);
}


/*--------------------------------------------------
 * Dump support to a generic OutputStream instance
 */
void Typelib::dump(Value v, OutputStream& stream)
{
    MemoryLayout ops;
    MemLayout::Visitor visitor(ops);
    visitor.apply(v.getType());
    return dump(v, stream, ops);
}
void Typelib::dump(Value v, OutputStream& stream, MemoryLayout const& ops)
{
    return dump(reinterpret_cast< boost::uint8_t* >(v.getData()), stream, ops);
}
void Typelib::dump(boost::uint8_t const* v, OutputStream& stream, MemoryLayout const& ops)
{
    MemoryLayout::const_iterator end = ValueOps::dump(
            v, 0, stream, ops.begin(), ops.end()).get<1>();
    if (end != ops.end())
        throw std::runtime_error("internal error in the marshalling process");
}


/*--------------------------------------------------
 * Dump support to char*
 *
 * It is not using the generic dump() methods as it will return 0 on buffer
 * overrun or marshalling error, instead of throwing an exception
 */
struct ByteArrayOutputStream : public OutputStream
{
    boost::uint8_t* buffer;
    unsigned int   buffer_size;
    unsigned int   current;
    ByteArrayOutputStream(boost::uint8_t* buffer, int buffer_size)
        : buffer(buffer), buffer_size(buffer_size), current(0) {}

    void write(boost::uint8_t const* data, size_t size)
    {
        if (current + size > buffer_size)
            throw std::exception();

        memcpy(&buffer[current], data, size);
        current += size;
    }
};
int Typelib::dump(Value v, boost::uint8_t* buffer, unsigned int buffer_size)
{
    MemoryLayout ops;
    MemLayout::Visitor visitor(ops);
    visitor.apply(v.getType());
    return dump(v, buffer, buffer_size, ops);
}

int Typelib::dump(Value v, boost::uint8_t* buffer, unsigned int buffer_size, MemoryLayout const& ops)
{
    return dump(reinterpret_cast< boost::uint8_t const* >(v.getData()), buffer, buffer_size, ops);
}
int Typelib::dump(boost::uint8_t const* v, boost::uint8_t* buffer, unsigned int buffer_size, MemoryLayout const& ops)
{
    ByteArrayOutputStream stream(buffer, buffer_size);
    MemoryLayout::const_iterator end;
    try {
        end = ValueOps::dump(
                v, 0, stream, ops.begin(), ops.end()).get<1>();
    }
    catch(std::exception const& e) {
        std::cout << "failed to marshal: " << e.what() << std::endl;
        return 0;
    }
    if (end != ops.end())
        throw std::runtime_error("internal error in the marshalling process");
    return stream.current;
}



/*--------------------------------------------------
 * Marshalling size calculations
 */
struct ByteCounterStream : public OutputStream
{
    size_t result;
    ByteCounterStream()
        : result(0) {}

    void write(boost::uint8_t const* data, size_t size)
    { result += size; }
};
size_t Typelib::getDumpSize(Value v)
{ 
    MemoryLayout ops;
    MemLayout::Visitor visitor(ops);
    visitor.apply(v.getType());
    return getDumpSize(v, ops);
}
size_t Typelib::getDumpSize(Value v, MemoryLayout const& ops)
{ return getDumpSize(reinterpret_cast< boost::uint8_t const* >(v.getData()), ops); }
size_t Typelib::getDumpSize(boost::uint8_t const* v, MemoryLayout const& ops)
{
    ByteCounterStream counter;
    ValueOps::dump(v, 0, counter, ops.begin(), ops.end());
    return counter.result;
}






/*--------------------------------------------------
 * Generic load support from InputStream
 */
void Typelib::load(Value v, InputStream& stream)
{
    MemoryLayout ops = layout_of(v.getType());
    return load(v, stream, ops);
}
void Typelib::load(Value v, InputStream& stream, MemoryLayout const& ops)
{ load(reinterpret_cast< boost::uint8_t* >(v.getData()), v.getType(), stream, ops); }
void Typelib::load(boost::uint8_t* v, Type const& type, InputStream& stream, MemoryLayout const& ops)
{
    MemoryLayout::const_iterator it;
    size_t out_offset;
    tie(out_offset, it) =
        ValueOps::load(v, 0, stream, ops.begin(), ops.end());
    if (it != ops.end())
        throw std::runtime_error("internal error in the memory layout");
}

/*--------------------------------------------------
 * Load support from std::vector< boost::uint8_t >
 */
struct VectorInputStream : public InputStream
{
    std::vector< boost::uint8_t > const& buffer;
    size_t in_index;

    VectorInputStream(std::vector< boost::uint8_t > const& buffer)
        : buffer(buffer), in_index(0) {}

    void read(boost::uint8_t* out_buffer, size_t size)
    {
        if (size + in_index > buffer.size())
            throw std::runtime_error("error in load(): not enough data as input, expected at least " + lexical_cast<string>(size + in_index) + " bytes but got " + lexical_cast<string>(buffer.size()));

        memcpy(&out_buffer[0], &buffer[in_index], size);
        in_index += size;
    }
};
void Typelib::load(Value v, std::vector< boost::uint8_t > const& buffer)
{
    MemoryLayout ops = layout_of(v.getType());
    return load(v, buffer, ops);
}
void Typelib::load(Value v, std::vector< boost::uint8_t > const& buffer, MemoryLayout const& ops)
{ load(reinterpret_cast< boost::uint8_t* >(v.getData()), v.getType(), buffer, ops); }
void Typelib::load(boost::uint8_t* v, Type const& type, std::vector< boost::uint8_t > const& buffer, MemoryLayout const& ops)
{
    MemoryLayout::const_iterator it;
    VectorInputStream stream(buffer);

    size_t out_offset;
    tie(out_offset, it) =
        ValueOps::load(v, 0, stream, ops.begin(), ops.end());
    if (it != ops.end())
        throw std::runtime_error("internal error in the memory layout");
    if (stream.in_index != buffer.size() && stream.in_index + type.getTrailingPadding() != buffer.size())
        throw std::runtime_error("parts of the provided buffer has not been used (used " + 
                lexical_cast<string>(stream.in_index) + " bytes, got " + lexical_cast<string>(buffer.size()) + "as input)");
}




/*--------------------------------------------------
 * Load support from uint8_t*
 */
struct ByteArrayInputStream : public InputStream
{
    boost::uint8_t const* buffer;
    unsigned int buffer_size;
    unsigned int in_index;

    ByteArrayInputStream(boost::uint8_t const* buffer, int buffer_size)
        : buffer(buffer), buffer_size(buffer_size), in_index(0) {}

    void read(boost::uint8_t* out_buffer, size_t size)
    {
        if (size + in_index > buffer_size)
            throw std::runtime_error("error in load(): not enough data as input, expected at least " + lexical_cast<string>(size + in_index) + " bytes but got " + lexical_cast<string>(buffer_size));

        memcpy(&out_buffer[0], &buffer[in_index], size);
        in_index += size;
    }
};
void Typelib::load(Value v, boost::uint8_t const* buffer, unsigned int buffer_size)
{
    MemoryLayout ops = layout_of(v.getType());
    return load(v, buffer, buffer_size, ops);
}
void Typelib::load(Value v, boost::uint8_t const* buffer, unsigned int buffer_size, MemoryLayout const& ops)
{ load(reinterpret_cast< boost::uint8_t* >(v.getData()), v.getType(), buffer, buffer_size, ops); }
void Typelib::load(boost::uint8_t* v, Type const& type, boost::uint8_t const* buffer, unsigned int buffer_size, MemoryLayout const& ops)
{
    MemoryLayout::const_iterator it;
    ByteArrayInputStream stream(buffer, buffer_size);

    size_t out_offset;
    tie(out_offset, it) =
        ValueOps::load(v, 0, stream, ops.begin(), ops.end());
    if (it != ops.end())
        throw std::runtime_error("internal error in the memory layout");
    if (stream.in_index != buffer_size && stream.in_index + type.getTrailingPadding() != buffer_size)
        throw std::runtime_error("parts of the provided buffer has not been used (used " + 
                lexical_cast<string>(stream.in_index) + " bytes, got " + lexical_cast<string>(buffer_size) + "as input)");
}


