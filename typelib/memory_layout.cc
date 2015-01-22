#include "memory_layout.hh"
#include <iostream>
using namespace Typelib;
using namespace Typelib::MemLayout;
using namespace std;

void MemoryLayout::removeTrailingSkips()
{
    while (ops.size() > 2 && ops[ops.size() - 2] == FLAG_SKIP)
    {
        ops.pop_back();
        ops.pop_back();
    }
}

bool MemoryLayout::isMemcpy() const
{
    return (ops.size() == 2 && ops[0] == MemLayout::FLAG_MEMCPY);
}
void MemoryLayout::pushMemcpy(size_t size)
{
    pushGenericOp(FLAG_MEMCPY, size);
}

void MemoryLayout::pushSkip(size_t size)
{
    pushGenericOp(FLAG_SKIP, size);
}

void MemoryLayout::pushEnd()
{
    ops.push_back(FLAG_END);
}

void MemoryLayout::pushGenericOp(size_t op, size_t size)
{
    ops.push_back(op);
    ops.push_back(size);
}

void MemoryLayout::pushArray(Array const& type, MemoryLayout const& array_ops)
{
    ops.push_back(FLAG_ARRAY);
    ops.push_back(type.getDimension());
    ops.insert(ops.end(), array_ops.begin(), array_ops.end());
    ops.push_back(FLAG_END);
}

void MemoryLayout::pushContainer(Container const& type, MemoryLayout const& container_ops)
{
    ops.push_back(FLAG_CONTAINER);
    ops.push_back(reinterpret_cast<size_t>(&type));
    ops.insert(ops.end(), container_ops.begin(), container_ops.end());
    ops.push_back(FLAG_END);
}

bool MemoryLayout::simplifyArray(size_t& memcpy_size, MemoryLayout& merged) const
{
    std::vector<size_t>& merged_ops = merged.ops;

    if (!merged_ops.empty())
    {
        size_t last_op = *(merged_ops.rbegin() + 1);
        if (last_op == FLAG_ARRAY)
        {
            memcpy_size *= merged_ops.back();
            merged_ops.pop_back();
            merged_ops.pop_back();
        }
        else return false;
    }

    if (!merged_ops.empty())
    {
        size_t last_op = *(merged_ops.rbegin() + 1);
        if (last_op == FLAG_MEMCPY)
        {
            memcpy_size += merged_ops.back();
            merged_ops.pop_back();
            merged_ops.pop_back();
        }
    }

    return true;
}

MemoryLayout MemoryLayout::simplify(bool merge_skip_copy) const
{
    // Merge skips and memcpy: if a skip is preceded by a memcpy (or another
    // skip), simply merge the counts.
    MemoryLayout merged;

    Ops::const_iterator it = ops.begin();
    Ops::const_iterator const end = ops.end();

    size_t current_op = FLAG_MEMCPY, current_op_count = 0;
    for (; it != end; ++it)
    {
        size_t op   = *it;

        if (op == FLAG_END && current_op == FLAG_MEMCPY && current_op_count)
        {
            bool simplified = simplifyArray(current_op_count, merged);
            if (!simplified)
            {
                merged.pushMemcpy(current_op_count);
                merged.pushEnd();
                current_op_count = 0;
            }
        }
        else if (op != FLAG_SKIP && op != FLAG_MEMCPY)
        {
            if (current_op_count)
            {
                merged.pushGenericOp(current_op, current_op_count);
                current_op_count = 0;
            }

            if (op == FLAG_END)
                merged.pushEnd();
            else
            {
                ++it;
                merged.pushGenericOp(op, *it);
            }
        }
        else if (merge_skip_copy)
        {
            current_op = FLAG_MEMCPY;
            current_op_count += *(++it);
        }
        else if (op != current_op)
        {
            if (current_op_count)
                merged.pushGenericOp(current_op, current_op_count);
            current_op = op;
            current_op_count = *(++it);
        }
        else
            current_op_count += *(++it);
    }

    if (current_op_count)
        merged.pushGenericOp(current_op, current_op_count);

    return merged;
}

void MemoryLayout::display(std::ostream& out) const
{
    std::string indent;
    for (const_iterator it = ops.begin(); it != ops.end(); ++it)
    {
        switch(*it)
        {
            case FLAG_MEMCPY:
                out << indent << "FLAG_MEMCPY " << *(++it) << "\n";
                break;
            case FLAG_SKIP:
                out << indent << "FLAG_SKIP " << *(++it) << "\n";
                break;
            case FLAG_ARRAY:
                out << indent << "FLAG_ARRAY " << *(++it) << "\n";
                indent += "  ";
                break;
            case FLAG_CONTAINER:
                out << indent << "FLAG_CONTAINER " << reinterpret_cast<Type const*>(*(++it))->getName() << "\n";
                indent += "  ";
                break;
            case FLAG_END:
                indent = indent.substr(0, indent.size() - 2);
                out << indent << "FLAG_END" << "\n";
                break;

        }
    }
}

MemoryLayout::Ops::const_iterator MemoryLayout::skipBlock(
        MemoryLayout::Ops::const_iterator begin,
        MemoryLayout::Ops::const_iterator end)
{
    size_t nesting = 0;
    for (MemoryLayout::const_iterator it = begin; it != end; ++it)
    {
        switch(*it)
        {
            case FLAG_ARRAY:
            case FLAG_CONTAINER:
                ++it;
                ++nesting;
                break;

            case FLAG_END:
                if (nesting == 0)
                    return it;
                --nesting;
                break;

            case FLAG_SKIP:
            case FLAG_MEMCPY:
                ++it;
            default:
                break;
        }
    }
    return end;
}

bool MemLayout::Visitor::visit_ (Numeric const& type)
{
    ops.pushMemcpy(type.getSize());
    return true;
}

bool MemLayout::Visitor::visit_ (Enum    const& type)
{
    ops.pushMemcpy(type.getSize());
    return true;
}
bool MemLayout::Visitor::visit_ (Array   const& type)
{
    MemoryLayout subops;
    MemLayout::Visitor array_visitor(subops);
    array_visitor.apply(type.getIndirection());
    ops.pushArray(type, subops);
    return true;
}
bool MemLayout::Visitor::visit_ (Container const& type)
{
    MemoryLayout subops;
    MemLayout::Visitor container_visitor(subops);
    container_visitor.apply(type.getIndirection());
    ops.pushContainer(type, subops);
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
        ops.pushSkip(it->getOffset() - current_offset);
        dispatch(it->getType());
        current_offset = it->getOffset() + it->getType().getSize();
    }
    ops.pushSkip(type.getSize() - current_offset);
    return true;
}
bool MemLayout::Visitor::visit_ (Pointer const& type)
{
    if (accept_pointers)
    {
        ops.pushMemcpy(type.getSize());
        return true;
    }
    else
        throw NoLayout(type, "is a pointer");
}
bool MemLayout::Visitor::visit_ (OpaqueType const& type)
{
    if (accept_opaques)
    {
        ops.pushSkip(type.getSize());
        return true;
    }
    else
        throw NoLayout(type, "is an opaque type");
}

MemLayout::Visitor::Visitor(MemoryLayout& ops, bool accept_pointers, bool accept_opaques)
    : ops(ops), accept_pointers(accept_pointers), accept_opaques(accept_opaques) {}

