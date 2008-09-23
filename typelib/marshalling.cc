#include "marshalling.hh"
#include <string.h>
#include <boost/tuple/tuple.hpp>
#include <iostream>
using namespace Typelib;

void MarshalOpsVisitor::push_current_memcpy()
{
    if (current_memcpy)
    {
        ops.push_back(FLAG_MEMCPY);
        ops.push_back(current_memcpy);
        current_memcpy = 0;
    }
}
void MarshalOpsVisitor::skip(size_t count)
{
    if (count == 0)
        return;

    if (current_memcpy)
        current_memcpy += count;
    else
    {
        ops.push_back(FLAG_SKIP);
        ops.push_back(count);
    }
}
bool MarshalOpsVisitor::generic_visit(Type const& value)
{
    current_memcpy += value.getSize();
    return true;
};
bool MarshalOpsVisitor::visit_ (Numeric const& type) { return generic_visit(type); }
bool MarshalOpsVisitor::visit_ (Enum    const& type) { return generic_visit(type); }
bool MarshalOpsVisitor::visit_ (Array   const& type)
{
    MarshalOps subops;
    MarshalOpsVisitor array_visitor(subops);
    array_visitor.apply(type.getIndirection());

    if (subops.size() == 2 && subops.front() == FLAG_MEMCPY)
        current_memcpy += subops.back() * type.getDimension();
    else
    {
        push_current_memcpy();
        ops.push_back(FLAG_ARRAY);
        ops.push_back(type.getDimension());
        ops.insert(ops.end(), subops.begin(), subops.end());
        ops.push_back(FLAG_END);
    }
}
bool MarshalOpsVisitor::visit_ (Container const& type)
{
    push_current_memcpy();
    ops.push_back(FLAG_CONTAINER);
    ops.push_back(reinterpret_cast<size_t>(&type));

    MarshalOpsVisitor container_visitor(ops);
    container_visitor.apply(type.getIndirection());

    ops.push_back(FLAG_END);
    return true;
}
bool MarshalOpsVisitor::visit_ (Compound const& type)
{
    typedef Compound::FieldList Fields;
    Fields const& fields(type.getFields());
    Fields::const_iterator const end = fields.end();

    size_t current_offset = 0;
    for (Fields::const_iterator it = fields.begin(); it != end; ++it)
    {
        skip(it->getOffset() - current_offset);
        dispatch(it->getType());
        current_offset = it->getOffset() + it->getType().getSize();
    }
    skip(type.getSize() - current_offset);
    return true;
}
bool MarshalOpsVisitor::visit_ (Pointer const& type) { throw UnsupportedMarshalling("pointers"); }
bool MarshalOpsVisitor::visit_ (OpaqueType const& type) { throw UnsupportedMarshalling("opaque types"); }

MarshalOpsVisitor::MarshalOpsVisitor(MarshalOps& ops)
    : current_memcpy(0), ops(ops) {}

void MarshalOpsVisitor::apply(Type const& type)
{
    current_memcpy = 0;
    TypeVisitor::apply(type);
    push_current_memcpy();
}

std::pair<MarshalOps::const_iterator, size_t> MarshalOpsVisitor::dump(
        uint8_t* data, size_t in_offset,
        std::vector<uint8_t>& buffer,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end)
{
    MarshalOps::const_iterator it;
    for (it = begin; it != end && *it != MarshalOpsVisitor::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MarshalOpsVisitor::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                int out_offset  = buffer.size();
                buffer.resize( out_offset + size );
                memcpy( &buffer[out_offset], data + in_offset, size);
                in_offset += size;
                break;
            }
            case MarshalOpsVisitor::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MarshalOps::const_iterator element_it = ++it;
                for (int i = 0; i < element_count; ++i)
                    boost::tie(it, in_offset) = dump(data, in_offset, buffer, element_it, end);

                if (it == end || *it != MarshalOpsVisitor::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode: array does not end with FLAG_END");

                break;
            }
            case MarshalOpsVisitor::FLAG_SKIP:
            {
                size_t size  = *(++it);
                in_offset += size;
                break;
            }
            case MarshalOpsVisitor::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                // First, dump the element count into the stream
                void* container_ptr  = data + in_offset;
                in_offset += type->getSize();

                size_t element_count = type->getElementCount(container_ptr);

                int out_offset  = buffer.size();
                buffer.resize( out_offset + sizeof(uint64_t) );
                reinterpret_cast<uint64_t&>(buffer[out_offset]) = element_count;

                it = type->dump(container_ptr, element_count, buffer, ++it, end);

                if (it == end || *it != MarshalOpsVisitor::FLAG_END)
                    throw std::runtime_error("error in bytecode while dumping: container does not end with FLAG_END");
                break;
            }
            default:
                throw UnknownMarshallingBytecode();
        }
    }

    return make_pair(it, in_offset);
}

boost::tuple<MarshalOps::const_iterator, size_t, size_t> MarshalOpsVisitor::load(
        uint8_t* data, size_t out_offset,
        std::vector<uint8_t> const& buffer, size_t in_offset,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end)
{
    MarshalOps::const_iterator it;
    for (it = begin; it != end && *it != MarshalOpsVisitor::FLAG_END; ++it)
    {
        switch(*it)
        {
            case MarshalOpsVisitor::FLAG_MEMCPY:
            {
                size_t size = *(++it);
                if (in_offset + size > buffer.size())
                    throw MarshalledTypeMismatch();

                memcpy(data + out_offset, &buffer[in_offset], size);
                in_offset  += size;
                out_offset += size;
                break;
            }
            case MarshalOpsVisitor::FLAG_ARRAY:
            {
                size_t element_count = *(++it);
                MarshalOps::const_iterator element_it = ++it;
                for (int i = 0; i < element_count; ++i)
                    boost::tie(it, out_offset, in_offset) = load(data, out_offset, buffer, in_offset, element_it, end);

                if (*it != MarshalOpsVisitor::FLAG_END)
                    throw std::runtime_error("error in the marshalling bytecode at array end");

                break;
            }
            case MarshalOpsVisitor::FLAG_SKIP:
            {
                size_t size  = *(++it);
                out_offset += size;
                break;
            }
            case MarshalOpsVisitor::FLAG_CONTAINER:
            {
                Container const* type = reinterpret_cast<Container const*>(*(++it));
                void* container_ptr  = data + out_offset;
                type->init(container_ptr);
                out_offset += type->getSize();

                size_t element_count = reinterpret_cast<uint64_t const&>(buffer[in_offset]);
                in_offset += sizeof(uint64_t);

                boost::tie(it, in_offset) =
                    type->load(container_ptr, element_count, buffer, in_offset, ++it, end);

                if (it == end || *it != MarshalOpsVisitor::FLAG_END)
                    throw std::runtime_error("error in bytecode while dumping: container does not end with FLAG_END");
                break;
            }
            default:
            {
                std::cerr << "loading: found " << *it << std::endl;
                throw UnknownMarshallingBytecode();
            }
        }
    }

    return boost::make_tuple(it, out_offset, in_offset);
}


std::vector<uint8_t> Typelib::dump_to_memory(Value v)
{
    std::vector<uint8_t> buffer;
    dump_to_memory(v, buffer);
    return buffer;
}

void Typelib::dump_to_memory(Value v, std::vector<uint8_t>& buffer)
{
    MarshalOps ops;
    MarshalOpsVisitor visitor(ops);
    visitor.apply(v.getType());
    return dump_to_memory(v, buffer, ops);
}

void Typelib::dump_to_memory(Value v, std::vector<uint8_t>& buffer, MarshalOps const& ops)
{
    int base_offset;
    MarshalOps::const_iterator end = MarshalOpsVisitor::dump(
            reinterpret_cast<uint8_t*>(v.getData()), 0,
            buffer, ops.begin(), ops.end()).first;
    if (end != ops.end())
        throw std::runtime_error("internal error in the marshalling process");
}



void Typelib::load_from_memory(Value v, std::vector<uint8_t> const& buffer)
{
    MarshalOps ops;
    MarshalOpsVisitor visitor(ops);
    visitor.apply(v.getType());
    return load_from_memory(v, buffer, ops);
}

void Typelib::load_from_memory(Value v, std::vector<uint8_t> const& buffer, MarshalOps const& ops)
{
    MarshalOps::const_iterator it;
    int in_offset, out_offset;
    boost::tie(it, out_offset, in_offset) =
        MarshalOpsVisitor::load(
                reinterpret_cast<uint8_t*>(v.getData()), 0,
                buffer, 0, ops.begin(), ops.end());
    if (it != ops.end())
        throw std::runtime_error("internal error in the marshalled operations");
    if (in_offset != buffer.size())
        throw std::runtime_error("parts of the provided buffer has not been used");
    if (out_offset != v.getType().getSize())
        throw MarshalledTypeMismatch();
}

