#include "memory_layout.hh"
using namespace Typelib;

void MemLayout::Visitor::push_current_memcpy()
{
    if (current_memcpy)
    {
        ops.push_back(FLAG_MEMCPY);
        ops.push_back(current_memcpy);
        current_memcpy = 0;
    }
}
void MemLayout::Visitor::skip(size_t count)
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
bool MemLayout::Visitor::generic_visit(Type const& value)
{
    current_memcpy += value.getSize();
    return true;
};
bool MemLayout::Visitor::visit_ (Numeric const& type) { return generic_visit(type); }
bool MemLayout::Visitor::visit_ (Enum    const& type) { return generic_visit(type); }
bool MemLayout::Visitor::visit_ (Array   const& type)
{
    MemoryLayout subops;
    MemLayout::Visitor array_visitor(subops);
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
bool MemLayout::Visitor::visit_ (Container const& type)
{
    push_current_memcpy();
    ops.push_back(FLAG_CONTAINER);
    ops.push_back(reinterpret_cast<size_t>(&type));

    MemLayout::Visitor container_visitor(ops);
    container_visitor.apply(type.getIndirection());

    ops.push_back(FLAG_END);
    return true;
}
bool MemLayout::Visitor::visit_ (Compound const& type)
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
bool MemLayout::Visitor::visit_ (Pointer const& type)
{
    if (accept_pointers)
        return generic_visit(type);
    else
        throw NoLayout(type, "is a pointer");
}
bool MemLayout::Visitor::visit_ (OpaqueType const& type)
{
    if (accept_opaques)
    {
        skip(type.getSize());
        return true;
    }
    else
        throw NoLayout(type, "is an opaque type");
}

MemLayout::Visitor::Visitor(MemoryLayout& ops, bool accept_pointers, bool accept_opaques)
    : ops(ops), accept_pointers(accept_pointers), accept_opaques(accept_opaques), current_memcpy(0) {}

void MemLayout::Visitor::apply(Type const& type)
{
    current_memcpy = 0;
    TypeVisitor::apply(type);
    push_current_memcpy();
}



MemoryLayout::const_iterator MemLayout::skip_block(
        MemoryLayout::const_iterator begin,
        MemoryLayout::const_iterator end)
{
    size_t nesting = 0;
    for (MemoryLayout::const_iterator it = begin; it != end; ++it)
    {
        switch(*it)
        {
            case FLAG_ARRAY:
            case FLAG_CONTAINER:
                ++nesting;
                break;

            case FLAG_END:
                if (nesting == 0)
                    return it;
                --nesting;
                break;

            case FLAG_SKIP:
            case FLAG_MEMCPY:
            default:
                break;
        }
    }
    return end;
}

