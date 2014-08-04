#include "memory_layout.hh"
using namespace Typelib;

void MemLayout::Visitor::push_current_op() {
    if (current_op_count) {
        ops.push_back(current_op);
        ops.push_back(current_op_count);
        current_op_count = 0;
    }
}
void MemLayout::Visitor::skip(size_t count) {
    add_generic_op(FLAG_SKIP, count);
}
void MemLayout::Visitor::memcpy(size_t count) {
    add_generic_op(FLAG_MEMCPY, count);
}

void MemLayout::Visitor::add_generic_op(size_t op, size_t size) {
    if (size == 0)
        return;
    if (current_op != op)
        push_current_op();

    current_op = op;
    current_op_count += size;
}

bool MemLayout::Visitor::generic_visit(Type const &value) {
    memcpy(value.getSize());
    return true;
};
bool MemLayout::Visitor::visit_(Numeric const &type) {
    return generic_visit(type);
}
bool MemLayout::Visitor::visit_(Enum const &type) {
    return generic_visit(type);
}
bool MemLayout::Visitor::visit_(Array const &type) {
    MemoryLayout subops;
    MemLayout::Visitor array_visitor(subops);
    array_visitor.apply(type.getIndirection(), merge_skip_copy, false);

    if (subops.size() == 2 && subops.front() == FLAG_MEMCPY)
        memcpy(subops.back() * type.getDimension());
    else {
        push_current_op();
        ops.push_back(FLAG_ARRAY);
        ops.push_back(type.getDimension());
        ops.insert(ops.end(), subops.begin(), subops.end());
        ops.push_back(FLAG_END);
    }
    return true;
}
bool MemLayout::Visitor::visit_(Container const &type) {
    push_current_op();
    ops.push_back(FLAG_CONTAINER);
    ops.push_back(reinterpret_cast<size_t>(&type));

    MemoryLayout subops;
    MemLayout::Visitor container_visitor(subops);
    container_visitor.apply(type.getIndirection(), merge_skip_copy, false);

    ops.insert(ops.end(), subops.begin(), subops.end());
    ops.push_back(FLAG_END);
    return true;
}
bool MemLayout::Visitor::visit_(Compound const &type) {
    typedef Compound::FieldList Fields;
    Fields const &fields(type.getFields());
    Fields::const_iterator const end = fields.end();

    size_t current_offset = 0;
    for (Fields::const_iterator it = fields.begin(); it != end; ++it) {
        skip(it->getOffset() - current_offset);
        dispatch(it->getType());
        current_offset = it->getOffset() + it->getType().getSize();
    }
    skip(type.getSize() - current_offset);
    return true;
}
bool MemLayout::Visitor::visit_(Pointer const &type) {
    if (accept_pointers)
        return generic_visit(type);
    else
        throw NoLayout(type, "is a pointer");
}
bool MemLayout::Visitor::visit_(OpaqueType const &type) {
    if (accept_opaques) {
        skip(type.getSize());
        return true;
    } else
        throw NoLayout(type, "is an opaque type");
}

MemLayout::Visitor::Visitor(MemoryLayout &ops, bool accept_pointers,
                            bool accept_opaques)
    : ops(ops), accept_pointers(accept_pointers),
      accept_opaques(accept_opaques), current_op(FLAG_MEMCPY),
      current_op_count(0) {}

void MemLayout::Visitor::apply(Type const &type, bool merge_skip_copy,
                               bool remove_trailing_skips) {
    this->merge_skip_copy = merge_skip_copy;

    current_op = FLAG_MEMCPY;
    current_op_count = 0;
    TypeVisitor::apply(type);
    push_current_op();

    // Remove trailing skips, they are useless
    if (remove_trailing_skips) {
        while (ops.size() > 2 && ops[ops.size() - 2] == FLAG_SKIP) {
            ops.pop_back();
            ops.pop_back();
        }
    }

    if (merge_skip_copy)
        merge_skips_and_copies();
}

void MemLayout::Visitor::merge_skips_and_copies() {
    // Merge skips and memcpy: if a skip is preceded by a memcpy (or another
    // skip), simply merge the counts.
    MemoryLayout merged;

    MemoryLayout::const_iterator it = ops.begin();
    MemoryLayout::const_iterator const end = ops.end();
    while (it != end) {
        size_t op = *it;
        if (op != FLAG_SKIP && op != FLAG_MEMCPY) {
            MemoryLayout::const_iterator element_it = it;
            ++(++(element_it));
            MemoryLayout::const_iterator block_end_it =
                skip_block(element_it, end);
            ++block_end_it;
            merged.insert(merged.end(), it, block_end_it);
            it = block_end_it;
            continue;
        }

        // Merge following FLAG_MEMCPY and FLAG_SKIP operations
        size_t size = *(++it);
        for (++it; it != end; ++it) {
            if (*it == FLAG_MEMCPY)
                op = FLAG_MEMCPY;
            else if (*it != FLAG_SKIP)
                break;

            size += *(++it);
        }
        merged.push_back(op);
        merged.push_back(size);
    }
    ops = merged;
}

MemoryLayout::const_iterator
MemLayout::skip_block(MemoryLayout::const_iterator begin,
                      MemoryLayout::const_iterator end) {
    size_t nesting = 0;
    for (MemoryLayout::const_iterator it = begin; it != end; ++it) {
        switch (*it) {
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
