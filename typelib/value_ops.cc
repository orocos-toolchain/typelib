#include <typelib/value_ops.hh>
#include <string.h>
#include <boost/lexical_cast.hpp>

using namespace Typelib;
using namespace boost;
using namespace std;

tuple<size_t, MemoryLayout::const_iterator> ValueOps::dump(
        uint8_t const* data, size_t in_offset,
        std::vector<uint8_t>& buffer,
        MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end)
{
    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                int out_offset  = buffer.size();
                buffer.resize( out_offset + size );
                memcpy( &buffer[out_offset], data + in_offset, size);
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
                        tie(in_offset, it) = dump(data, in_offset, buffer, element_it, end);
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

                size_t element_count = type->getElementCount(container_ptr);

                int out_offset = buffer.size();
                buffer.resize( out_offset + sizeof(uint64_t) );
                reinterpret_cast<uint64_t&>(buffer[out_offset]) = element_count;

                if (element_count == 0)
                    it = MemLayout::skip_block(++it, end);
                else
                    it = type->dump(container_ptr, element_count, buffer, ++it, end);

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

tuple<size_t, size_t, MemoryLayout::const_iterator> ValueOps::load(
        uint8_t* data, size_t out_offset,
        std::vector<uint8_t> const& buffer, size_t in_offset,
        MemoryLayout::const_iterator const begin, MemoryLayout::const_iterator const end)
{
    MemoryLayout::const_iterator it;
    for (it = begin; it != end && *it != MemLayout::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                if (in_offset + size > buffer.size())
                    throw std::runtime_error("in load(): input buffer too small");

                memcpy(data + out_offset, &buffer[in_offset], size);
                in_offset  += size;
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
                        tie(out_offset, in_offset, it) =
                            load(data, out_offset, buffer, in_offset, element_it, end);
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
                type->init(container_ptr);
                out_offset += type->getSize();

                size_t element_count = reinterpret_cast<uint64_t const&>(buffer[in_offset]);
                in_offset += sizeof(uint64_t);
                if (element_count == 0)
                    it = MemLayout::skip_block(++it, end);
                else
                {
                    tie(in_offset, it) =
                        type->load(container_ptr, element_count, buffer, in_offset, ++it, end);
                }

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("bytecode error in load(): container does not end with FLAG_END");
                break;
            }
            default:
                throw UnknownLayoutBytecode();
        }
    }

    return make_tuple(out_offset, in_offset, it);
}


void Typelib::init(Value v)
{
    uint8_t* buffer = reinterpret_cast<uint8_t*>(v.getData());
    MemoryLayout ops = layout_of(v.getType(), true);
    ValueOps::init(buffer, ops.begin(), ops.end());
}

void Typelib::destroy(Value v)
{
    uint8_t* buffer = reinterpret_cast<uint8_t*>(v.getData());
    MemoryLayout ops = layout_of(v.getType(), true);
    ValueOps::destroy(buffer, ops.begin(), ops.end());
}

void Typelib::destroy(Value v, MemoryLayout const& ops)
{
    ValueOps::destroy(reinterpret_cast<uint8_t*>(v.getData()),
            ops.begin(), ops.end());
}

void Typelib::copy(Value dst, Value src)
{
    if (dst.getType() != src.getType())
        throw std::runtime_error("requested copy with incompatible types");

    uint8_t* out_buffer = reinterpret_cast<uint8_t*>(dst.getData());
    uint8_t* in_buffer  = reinterpret_cast<uint8_t*>(src.getData());
    MemoryLayout ops = layout_of(dst.getType());
    ValueOps::copy(out_buffer, in_buffer, ops.begin(), ops.end());
}

bool Typelib::compare(Value dst, Value src)
{
    if (dst.getType() != src.getType())
        return false;

    uint8_t* out_buffer = reinterpret_cast<uint8_t*>(dst.getData());
    uint8_t* in_buffer  = reinterpret_cast<uint8_t*>(src.getData());
    MemoryLayout ops = layout_of(dst.getType());

    bool is_equal;
    tie(is_equal, tuples::ignore, tuples::ignore, tuples::ignore) =
        ValueOps::compare(out_buffer, in_buffer, ops.begin(), ops.end());
    return is_equal;
}
tuple<uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::init(uint8_t* buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end; ++it)
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
                throw std::runtime_error("unrecognized marshalling bytecode");
        }
    }

    return make_tuple(buffer, it);
}

tuple<uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::destroy(uint8_t* buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{
    MemoryLayout::const_iterator it;
    for (it = begin; it != end; ++it)
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
                throw std::runtime_error("unrecognized marshalling bytecode");
        }
    }

    return make_tuple(buffer, it);
}

tuple<uint8_t*, uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::copy(uint8_t* out_buffer, uint8_t* in_buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end; ++it)
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
                throw std::runtime_error("unrecognized marshalling bytecode");
        }
    }

    return make_tuple(out_buffer, in_buffer, it);
}

tuple<bool, uint8_t*, uint8_t*, MemoryLayout::const_iterator>
    Typelib::ValueOps::compare(uint8_t* out_buffer, uint8_t* in_buffer,
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{

    MemoryLayout::const_iterator it;
    for (it = begin; it != end; ++it)
    {
        switch(*it)
        {
            case MemLayout::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                if (memcmp(out_buffer, in_buffer, size))
                    return make_tuple(false, (uint8_t*)0, (uint8_t*)0, end);

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
                        return make_tuple(false, (uint8_t*)0, (uint8_t*)0, end);
                }

                if (it == end || *it != MemLayout::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");
                break;
            }
            case MemLayout::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                if (!type->compare(out_buffer, in_buffer))
                    return make_tuple(false, (uint8_t*)0, (uint8_t*)0, end);

                it = MemLayout::skip_block(it, end);
                out_buffer += type->getSize();
                in_buffer  += type->getSize();
                break;
            }
            default:
                throw std::runtime_error("unrecognized marshalling bytecode");
        }
    }
    return make_tuple(true, out_buffer, in_buffer, it);
}



std::vector<uint8_t> Typelib::dump(Value v)
{
    std::vector<uint8_t> buffer;
    dump(v, buffer);
    return buffer;
}

void Typelib::dump(Value v, std::vector<uint8_t>& buffer)
{
    MemoryLayout ops;
    MemLayout::Visitor visitor(ops);
    visitor.apply(v.getType());
    return dump(v, buffer, ops);
}

void Typelib::dump(Value v, std::vector<uint8_t>& buffer, MemoryLayout const& ops)
{
    dump(reinterpret_cast<uint8_t const*>(v.getData()), buffer, ops);
}

void Typelib::dump(uint8_t const* v, std::vector<uint8_t>& buffer, MemoryLayout const& ops)
{
    MemoryLayout::const_iterator end = ValueOps::dump(
            v, 0, buffer, ops.begin(), ops.end()).get<1>();
    if (end != ops.end())
        throw std::runtime_error("internal error in the marshalling process");
}

void Typelib::load(Value v, std::vector<uint8_t> const& buffer)
{
    MemoryLayout ops = layout_of(v.getType());
    return load(v, buffer, ops);
}

void Typelib::load(Value v, std::vector<uint8_t> const& buffer, MemoryLayout const& ops)
{ load(reinterpret_cast<uint8_t*>(v.getData()), buffer, ops); }

void Typelib::load(uint8_t* v, std::vector<uint8_t> const& buffer, MemoryLayout const& ops)
{
    MemoryLayout::const_iterator it;
    size_t in_offset, out_offset;
    tie(out_offset, in_offset, it) =
        ValueOps::load(v, 0, buffer, 0, ops.begin(), ops.end());
    if (it != ops.end())
        throw std::runtime_error("internal error in the memory layout");
    if (in_offset != buffer.size())
        throw std::runtime_error("parts of the provided buffer has not been used (used " + 
                lexical_cast<string>(in_offset) + " bytes, got " + lexical_cast<string>(buffer.size()) + "as input)");
}


