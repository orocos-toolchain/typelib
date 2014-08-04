#ifndef TYPELIB_MEMORY_LAYOUT_HH
#define TYPELIB_MEMORY_LAYOUT_HH

#include <stdexcept>
#include <typelib/typevisitor.hh>
#include <vector>
#include <boost/static_assert.hpp>

namespace Typelib {
/** Exception raised when generating a memory layout for an unsupported type
 */
struct NoLayout : public std::runtime_error {
    NoLayout(Type const &type, std::string const &reason)
        : std::runtime_error("there is no memory layout for type " +
                             type.getName() + ": " + reason) {}
};

/** Thrown in operations that are using memlayout bytecode and found an
 * invalid bytecode.
 */
struct UnknownLayoutBytecode : public std::runtime_error {
    UnknownLayoutBytecode()
        : std::runtime_error(
              "found an unknown marshalling bytecode operation") {}
};

// This is needed because the FLAG_CONTAINER descriptor is followed by a
// pointer on a Container instance
BOOST_STATIC_ASSERT((sizeof(size_t) == sizeof(void *)));

typedef std::vector<size_t> MemoryLayout;

/** Namespace used to define basic constants describing the memory layout
 * of a type. The goal is to have a compact representation, in a buffer, of
 * what is raw data, what is junk and what is Container objects.
 *
 * This is used by most Value operations (init, ...)
 */
namespace MemLayout {
/** Valid marshalling operations
 *
 * FLAG_MEMCPY    <byte count>
 * FLAG_SKIP      <byte count>
 * FLAG_ARRAY     <element count>  [marshalling ops for pointed-to type]
 *FLAG_END
 * FLAG_CONTAINER <pointer-to-Container-object> [marshalling ops for pointed-to
 *type] FLAG_END
 */
enum Operations {
    FLAG_MEMCPY,
    FLAG_ARRAY,
    FLAG_CONTAINER,
    FLAG_SKIP,
    FLAG_END
};

/** This visitor computes the memory layout for a given type. This memory
 * layout is used by quite a few memory operations
 *
 * You should not have to use this directly. Use
 * Typelib.layout_of(type) instead
 */
class Visitor : public TypeVisitor {
  private:
    MemoryLayout &ops;
    bool accept_pointers;
    bool accept_opaques;
    size_t current_op;
    size_t current_op_count;
    bool merge_skip_copy;

  protected:
    void push_current_op();
    void skip(size_t count);
    void memcpy(size_t count);
    void add_generic_op(size_t op, size_t count);
    bool generic_visit(Type const &value);
    bool visit_(Numeric const &type);
    bool visit_(Enum const &type);
    bool visit_(Array const &type);
    bool visit_(Container const &type);
    bool visit_(Compound const &type);
    bool visit_(Pointer const &type);
    bool visit_(OpaqueType const &type);

    void merge_skips_and_copies();

  public:
    Visitor(MemoryLayout &ops, bool accept_pointers = false,
            bool accept_opaques = false);

    void apply(Type const &type, bool merge_skip_copy = true,
               bool remove_trailing_skips = true);
};

/** Skips the block starting at \c begin. \c end is the end of the
 * complete layout (to avoid invalid memory accesses if the layout is
 * incorrect)
 *
 * It returns the position of the block's FLAG_END
 */
MemoryLayout::const_iterator skip_block(MemoryLayout::const_iterator begin,
                                        MemoryLayout::const_iterator end);
}

/** Returns the memory layout of the given type
 *
 * @param accept_pointers if false (the default), will throw a NoLayout
 * exception if the type given contains pointers. If true, they will simply
 * be skipped
 * @param accept_opaques if false (the default), will throw a NoLayout
 * exception if the type given contains opaques. If true, they will simply
 * be skipped
 * @param merge_skip_copy in a layout, zones that contain data are copied,
 * while zones that are there because of C++ padding rules are skipped. If
 * this is true (the default), consecutive copy/skips are merged into one
 * bigger copy, as doine one big memcpy() is probably more efficient than
 * skipping the few padding bytes. Set to false to turn that off.
 * @param remove_trailing_skips because of C/C++ padding rules, structures
 * might contain trailing bytes that don't contain information. If this
 * option is true (the default), these bytes are removed from the layout.
 */
inline MemoryLayout layout_of(Type const &t, bool accept_pointers = false,
                              bool accept_opaques = false,
                              bool merge_skip_copy = true,
                              bool remove_trailing_skips = true) {
    MemoryLayout ret;
    MemLayout::Visitor visitor(ret, accept_pointers, accept_opaques);
    visitor.apply(t, merge_skip_copy, remove_trailing_skips);
    return ret;
}
}

#endif
