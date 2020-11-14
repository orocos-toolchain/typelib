#include "memory_layout.hh"
#include <iostream>
#include <boost/lexical_cast.hpp>
using namespace Typelib;
using namespace Typelib::MemLayout;
using namespace std;
using boost::lexical_cast;

static_assert(sizeof(Enum::integral_type) <= sizeof(size_t), "Typelib's internal integral type must fit in a size_t");

bool MemoryLayout::isMemcpy() const
{
    return (ops.size() == 2 && ops[0] == MemLayout::FLAG_MEMCPY);
}
void MemoryLayout::pushMemcpy(size_t size, vector<uint8_t> const& init_data)
{
    ops.push_back(FLAG_MEMCPY);
    ops.push_back(size);

    if (init_data.empty()) {
        pushInitSkip(size);
    }
    else
    {
        if (size != init_data.size()) {
            throw runtime_error(
                "not enough or too many bytes provided as initialization data"
            );
        }
        pushInit(init_data);
    }
}

void MemoryLayout::pushSkip(size_t size)
{
    ops.push_back(FLAG_SKIP);
    ops.push_back(size);
    pushInitSkip(size);
}

void MemoryLayout::pushEnd()
{
    ops.push_back(FLAG_END);
}

void MemoryLayout::pushGenericOp(size_t op, size_t element)
{
    ops.push_back(op);
    ops.push_back(element);
}

void MemoryLayout::pushArray(Array const& type, MemoryLayout const& array_ops)
{
    return pushArray(type.getDimension(), array_ops);
}

void MemoryLayout::pushArray(size_t dimension, MemoryLayout const& array_ops)
{
    ops.push_back(FLAG_ARRAY);
    ops.push_back(dimension);
    ops.insert(ops.end(), array_ops.begin(), array_ops.end());
    ops.push_back(FLAG_END);

    pushInitRepeat(dimension, array_ops);
}

void MemoryLayout::pushContainer(Container const& type, MemoryLayout const& container_ops)
{
    ops.push_back(FLAG_CONTAINER);
    ops.push_back(reinterpret_cast<size_t>(&type));
    ops.insert(ops.end(), container_ops.begin(), container_ops.end());
    ops.push_back(FLAG_END);

    pushInitContainer(type);
}

void MemoryLayout::pushInit(std::vector<uint8_t> const& data)
{
    pushInitGenericOp(FLAG_INIT, data.size());
    init_ops.insert(init_ops.end(), data.begin(), data.end());
}

void MemoryLayout::pushInitSkip(size_t size)
{
    pushInitGenericOp(FLAG_INIT_SKIP, size);
}

void MemoryLayout::pushInitGenericOp(size_t op, size_t size)
{
    init_ops.push_back(op);
    init_ops.push_back(size);
}

void MemoryLayout::pushInitEnd()
{
    init_ops.push_back(FLAG_INIT_END);
}

void MemoryLayout::pushInitRepeat(size_t dimension, MemoryLayout const& array_ops)
{
    pushInitGenericOp(FLAG_INIT_REPEAT, dimension);
    init_ops.insert(init_ops.end(), array_ops.init_begin(), array_ops.init_end());
    pushInitEnd();
}

void MemoryLayout::pushInitContainer(Container const& container)
{
    pushInitGenericOp(FLAG_INIT_CONTAINER, reinterpret_cast<Ops::value_type>(&container));
}

MemoryLayout::Ops MemoryLayout::simplifyInit() const
{
    MemoryLayout result;
    const_iterator end = simplifyInit(init_begin(), init_end(), result, true);
    if (end != init_end())
        throw InvalidMemoryLayout("simplifyInit() did not reach the end");

    MemoryLayout::Ops& simplified = result.init_ops;
    if (simplified.size() == 2 && simplified[0] == FLAG_INIT_SKIP)
        simplified.clear();
    return simplified;
}

MemoryLayout::const_iterator MemoryLayout::simplifyInitBlock(const_iterator it, const_iterator end, MemoryLayout& result) const
{
    const_iterator simplify_end = simplifyInit(it, end, result);
    if (simplify_end == end)
        throw InvalidMemoryLayout("expected FLAG_INIT_END but reached end of stream");
    else if (*simplify_end != FLAG_INIT_END)
        throw InvalidMemoryLayout("expected FLAG_INIT_END got something else");
    return simplify_end;
}

MemoryLayout::const_iterator MemoryLayout::simplifyInit(const_iterator it, const_iterator end, MemoryLayout& simplified, bool shave_last_skip) const
{
    size_t current_op_count = 0;
    std::vector<uint8_t> current_init_data;
    for (; it != end; ++it)
    {
        size_t op   = *it;
        if (op == FLAG_INIT_END)
        {
            if (current_op_count)
                simplified.pushInitSkip(current_op_count);
            else if (!current_init_data.empty())
                simplified.pushInit(current_init_data);
            return it;
        }

        if (op == FLAG_INIT)
        {
            if (current_op_count)
            {
                simplified.pushInitSkip(current_op_count);
                current_op_count = 0;
            }
            size_t init_size = *(it + 1);
            current_init_data.insert(current_init_data.end(), it + 2, it + 2 + init_size);
            it += 1 + init_size;
            continue;
        }

        if (!current_init_data.empty())
        {
            simplified.pushInit(current_init_data);
            current_init_data.clear();
        }

        if (op == FLAG_INIT_REPEAT)
        {
            if (current_op_count)
                simplified.pushInitSkip(current_op_count);

            size_t array_size = *(++it);
            simplified.pushInitGenericOp(FLAG_INIT_REPEAT, array_size);

            size_t content_i = simplified.init_size();
            it = simplifyInitBlock(++it, end, simplified);
            size_t added_ops = simplified.init_size() - content_i;
            if (added_ops == 2 && simplified.init_ops[content_i] == FLAG_INIT_SKIP)
            {
                size_t count = simplified.init_ops[content_i + 1] * array_size;
                if (current_op_count)
                {
                    current_op_count += count;
                    simplified.init_ops.resize(content_i - 4);
                }
                else
                {
                    current_op_count = count;
                    simplified.init_ops.resize(content_i - 2);
                }
            }
            else if (added_ops == 0)
            {
                // Void all the modifications we've made so far
                simplified.init_ops.resize(content_i - 4);
            }
            else
            {
                simplified.pushInitEnd();
                current_op_count = 0;
            }
        }
        else if (op == FLAG_INIT_CONTAINER)
        {
            if (current_op_count)
            {
                simplified.pushInitSkip(current_op_count);
                current_op_count = 0;
            }
            simplified.pushInitGenericOp(FLAG_INIT_CONTAINER, *(++it));
        }
        else if (op == FLAG_INIT_SKIP)
        {
            current_op_count += *(++it);
        }
        else
            throw InvalidMemoryLayout("found invalid INIT bytecode");
    }

    if (current_op_count && !shave_last_skip)
    {
        std::cout << "adding " << current_op_count << " " << shave_last_skip << std::endl;
        simplified.pushInitSkip(current_op_count);
    }
    else if (!current_init_data.empty())
        simplified.pushInit(current_init_data);

    return it;
}

MemoryLayout MemoryLayout::simplify(bool merge_skip_copy, bool remove_trailing_skips) const
{
    // Merge skips and memcpy: if a skip is preceded by a memcpy (or another
    // skip), simply merge the counts.
    MemoryLayout merged;
    const_iterator end = simplify(merge_skip_copy, remove_trailing_skips, ops.begin(), ops.end(), 0, merged);
    if (end != ops.end())
        throw InvalidMemoryLayout("simplify() did not reach the end");

    merged.init_ops = simplifyInit();
    return merged;
}

MemoryLayout::const_iterator MemoryLayout::simplifyBlock(bool merge_skip_copy, bool remove_trailing_skips, const_iterator it, const_iterator end, unsigned int depth, MemoryLayout& simplified) const
{
    const_iterator simplify_end = simplify(merge_skip_copy, remove_trailing_skips, it, end, depth, simplified);
    if (simplify_end == end)
        throw InvalidMemoryLayout("expected FLAG_END but reached end of stream");
    else if (*simplify_end != FLAG_END)
        throw InvalidMemoryLayout("expected FLAG_END got something else");
    return simplify_end;
}

MemoryLayout::const_iterator MemoryLayout::simplify(bool merge_skip_copy, bool remove_trailing_skips, const_iterator it, const_iterator end, unsigned int depth, MemoryLayout& simplified) const
{
    size_t current_op = FLAG_MEMCPY, current_op_count = 0, current_skip_count = 0;
    for (; it != end; ++it)
    {
        size_t op   = *it;
        if (op == FLAG_END)
        {
            if (current_op_count)
                simplified.pushGenericOp(current_op, current_op_count);
            return it;
        }

        if (op == FLAG_ARRAY)
        {
            if (current_op_count)
                simplified.pushGenericOp(current_op, current_op_count);

            size_t array_size = *(++it);
            simplified.pushGenericOp(FLAG_ARRAY, array_size);
            size_t content_i = simplified.size();
            it = simplifyBlock(merge_skip_copy, remove_trailing_skips, ++it, end, depth + 1, simplified);
            size_t added_ops = simplified.size() - content_i;

            // Check whether we should squash the array and its contents.  Note
            // that the only ops that have one operand are FLAG_SKIP and
            // FLAG_MEMCPY
            if (added_ops == 2)
            {
                size_t op    = simplified.ops[content_i];
                size_t count = simplified.ops[content_i + 1] * array_size;

                // We check whether the operand just before was the same op, in
                // which case we squash them together as well. Otherwise,
                // recursive patterns such as
                //   array
                //      memcpy
                //      array
                //         mempcy
                //
                // would not be simplified properly
                //
                // Note that we don't squash array with array, as that will be
                // done when we reach the FLAG_END of the outermost array
                if (op == current_op && current_op_count)
                {
                    current_op_count += count;
                    simplified.ops.resize(content_i - 4);
                }
                else
                {
                    current_op = op;
                    current_op_count = count;
                    simplified.ops.resize(content_i - 2);
                }
            }
            else if (added_ops == 0)
            {
                // Void all the modifications we've made so far
                simplified.ops.resize(content_i - 4);
            }
            else
            {
                simplified.pushEnd();
                current_op_count = 0;
            }
        }
        else if (op == FLAG_CONTAINER)
        {
            if (current_op_count)
            {
                simplified.pushGenericOp(current_op, current_op_count);
                current_op_count = 0;
            }

            simplified.pushGenericOp(FLAG_CONTAINER, *(++it));
            it = simplifyBlock(merge_skip_copy, remove_trailing_skips, ++it, end, depth + 1, simplified);
            simplified.pushEnd();
        }
        else // either FLAG_MEMCPY or FLAG_SKIP
        {
            size_t size = *(++it);
            if (op == FLAG_SKIP)
                current_skip_count += size;

            if (op == current_op)
                current_op_count += size;
            else if (merge_skip_copy)
            {
                current_op = FLAG_MEMCPY;
                current_op_count += size;
            }
            else
            {
                if (current_op_count)
                    simplified.pushGenericOp(current_op, current_op_count);
                current_op = op;
                current_op_count = size;
            }
        }
        if (op != FLAG_SKIP)
            current_skip_count = 0;
    }

    if (current_op_count)
    {
        if (!remove_trailing_skips || current_op != FLAG_SKIP || depth > 0)
            simplified.pushGenericOp(current_op, current_op_count - current_skip_count);
    }

    return it;
}

void MemoryLayout::validate() const
{
    size_t indent = 0;
    for (const_iterator it = ops.begin(); it != ops.end(); ++it)
    {
        switch(*it)
        {
            case FLAG_MEMCPY:
                ++it;
                break;
            case FLAG_SKIP:
                ++it;
                break;
            case FLAG_ARRAY:
                ++it;
                ++indent;
                break;
            case FLAG_CONTAINER:
                ++it;
                ++indent;
                break;
            case FLAG_END:
                if (indent == 0)
                    throw InvalidMemoryLayout("found FLAG_END without a corresponding start (ARRAY/CONTAINER)");
                --indent;
                break;
            default:
                throw InvalidMemoryLayout("found invalid bytecode");

        }
    }

    if (indent > 0)
        throw InvalidMemoryLayout("missing FLAG_END: probably an unbalanced ARRAY/CONTAINER");

    for (const_iterator it = init_ops.begin(); it != init_ops.end(); ++it)
    {
        switch(*it)
        {
            case FLAG_INIT_SKIP:
                ++it;
                break;
            case FLAG_INIT:
            {
                size_t size = *(++it);
                if ((init_ops.end() - it) < static_cast<int>(size))
                {
                    throw InvalidMemoryLayout("found a FLAG_INIT block of size " +
                            lexical_cast<string>(size) + ", but there is only " +
                            lexical_cast<string>(init_ops.end() - it) + " instructions left in stream");
                }
                it += size;
                break;
            }
            case FLAG_INIT_REPEAT:
                ++it;
                ++indent;
                break;
            case FLAG_INIT_CONTAINER:
                ++it;
                break;
            case FLAG_INIT_END:
                if (indent == 0)
                    throw InvalidMemoryLayout("found FLAG_INIT_END without a corresponding FLAG_INIT_REPEAT");
                --indent;
                break;

        }
    }

    if (indent > 0)
        throw InvalidMemoryLayout("missing FLAG_INIT_END, probably an unbalanced FLAG_INIT_REPEAT");
}

void MemoryLayout::display(ostream& out) const
{
    string indent;
    for (const_iterator it = ops.begin(); it != ops.end(); ++it)
    {
        std::string idx = boost::lexical_cast<std::string>(it - ops.begin());
        if (idx.size() < 3)
            idx += "  ";
        if (idx.size() < 2)
            idx += " ";

        switch(*it)
        {
            case FLAG_MEMCPY:
                out << idx << indent << "FLAG_MEMCPY " << *(++it) << "\n";
                break;
            case FLAG_SKIP:
                out << idx << indent << "FLAG_SKIP " << *(++it) << "\n";
                break;
            case FLAG_ARRAY:
                out << idx << indent << "FLAG_ARRAY " << *(++it) << "\n";
                indent += "  ";
                break;
            case FLAG_CONTAINER:
                out << idx << indent << "FLAG_CONTAINER " << reinterpret_cast<Type const*>(*(++it))->getName() << "\n";
                indent += "  ";
                break;
            case FLAG_END:
                indent = indent.substr(0, indent.size() - 2);
                out << idx << indent << "FLAG_END" << "\n";
                break;

        }
    }

    if (!indent.empty())
        throw InvalidMemoryLayout("invalid memory layout: probably an unbalanced ARRAY/CONTAINER and END");

    indent = "";

    for (const_iterator it = init_ops.begin(); it != init_ops.end(); ++it)
    {
        switch(*it)
        {
            case FLAG_INIT_SKIP:
                out << indent << "FLAG_INIT_SKIP " << *(++it) << "\n";
                break;
            case FLAG_INIT:
            {
                size_t size = *(++it);
                out << indent << "FLAG_INIT " << size;
                for (size_t i = 0; i < size; ++i)
                    out << " " << hex << "0x" << *(++it);
                out << dec << "\n";
                break;
            }
            case FLAG_INIT_REPEAT:
                out << indent << "FLAG_INIT_REPEAT " << *(++it) << "\n";
                indent += "  ";
                break;
            case FLAG_INIT_CONTAINER:
                out << indent << "FLAG_INIT_CONTAINER " << reinterpret_cast<Type const*>(*(++it))->getName() << "\n";
                break;
            case FLAG_INIT_END:
                indent = indent.substr(0, indent.size() - 2);
                out << indent << "FLAG_INIT_END" << "\n";
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
    if (type.getSize() > 8)
        throw runtime_error("cannot handle enum types bigger than 8 bytes");

    vector<uint8_t> init_data;
    if (!type.values().empty())
    {
        uint64_t init_value = type.values().begin()->second;
        init_data.resize(type.getSize());
        for (size_t i = 0; i < type.getSize(); ++i) {
            init_data[i] = init_value;
            init_value >>= 8;
        }
    }
    ops.pushMemcpy(type.getSize(), init_data);

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

