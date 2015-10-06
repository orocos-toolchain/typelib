#ifndef TYPELIB_MEMORY_LAYOUT_HH
#define TYPELIB_MEMORY_LAYOUT_HH

#include <stdexcept>
#include <typelib/typevisitor.hh>
#include <vector>
#include <boost/static_assert.hpp>

namespace Typelib
{
    /** Exception raised when generating a memory layout for an unsupported type
     */
    struct NoLayout : public std::runtime_error
    { 
        NoLayout(Type const& type, std::string const& reason)
            : std::runtime_error("there is no memory layout for type " + type.getName() + ": " + reason) { }
    };

    /** Thrown in operations that are using memlayout bytecode and found an
     * invalid bytecode.
     */
    struct UnknownLayoutBytecode : public std::runtime_error
    { 
        UnknownLayoutBytecode() : std::runtime_error("found an unknown marshalling bytecode operation") { }
    };

    /** Exception thrown when processing a MemoryLayout and it appears that it
     * is invalid
     */
    class InvalidMemoryLayout : public std::runtime_error
    {
        public:
            InvalidMemoryLayout(std::string const& msg)
                : std::runtime_error(msg) {}
    };

    // This is needed because the FLAG_CONTAINER descriptor is followed by a
    // pointer on a Container instance
    BOOST_STATIC_ASSERT((sizeof(size_t) == sizeof(void*)));

    struct MemoryLayout
    {
        typedef std::vector<size_t> Ops;
        typedef Ops::const_iterator const_iterator;
        Ops ops;
        const_iterator begin() const { return ops.begin(); }
        const_iterator end() const   { return ops.end(); }
        size_t size() const { return ops.size(); }

        Ops init_ops;
        const_iterator init_begin() const { return init_ops.begin(); }
        const_iterator init_end() const   { return init_ops.end(); }
        size_t init_size() const { return init_ops.size(); }

        void removeTrailingSkips();
        void pushMemcpy(size_t size,
                std::vector<uint8_t> const& init_data = std::vector<uint8_t>());
        void pushSkip(size_t size);
        void pushArray(size_t dimension, MemoryLayout const& array_ops);
        void pushArray(Array const& type, MemoryLayout const& array_ops);
        void pushContainer(Container const& type, MemoryLayout const& container_ops);
        void pushGenericOp(size_t op, size_t size);
        void pushEnd();
        bool isMemcpy() const;
        MemoryLayout simplify(bool merge_skip_copy) const;
        void validate() const;
        void display(std::ostream& out) const;

        void pushInit(std::vector<uint8_t> const& data);
        void pushInitSkip(size_t size);
        void pushInitGenericOp(size_t op, size_t size);
        void pushInitRepeat(size_t dimension, MemoryLayout const& array_ops);
        void pushInitContainer(Container const& container);
        void pushInitEnd();

        /** Skips the block starting at \c begin. \c end is the end of the
         * complete layout (to avoid invalid memory accesses if the layout is
         * incorrect)
         *
         * It returns the position of the block's FLAG_END
         */
        static const_iterator skipBlock(
                const_iterator begin,
                const_iterator end);

    private:
        const_iterator simplify(bool merge_skip_copy, const_iterator it, const_iterator end, MemoryLayout& simplified) const;
        const_iterator simplifyBlock(bool merge_skip_copy, const_iterator it, const_iterator end, MemoryLayout& simplified) const;
        const_iterator simplifyInit(const_iterator it, const_iterator end, MemoryLayout& result, bool shave_last_skip = false) const;
        const_iterator simplifyInitBlock(const_iterator it, const_iterator end, MemoryLayout& result) const;
        size_t m_offset;
        bool simplifyArray(size_t& memcpy_size, MemoryLayout& merged_ops) const;
        bool simplifyInitRepeat(size_t& size, Ops& result) const;
        MemoryLayout::Ops simplifyInit() const;
    };

    /** Namespace used to define basic constants describing the memory layout
     * of a type. The goal is to have a compact representation, in a buffer, of
     * what is raw data, what is junk and what is Container objects.
     *
     * This is used by most Value operations (init, ...)
     */
    namespace MemLayout
    {
        /** Valid marshalling operations
         *
         * FLAG_MEMCPY    <byte count>
         * FLAG_SKIP      <byte count>
         * FLAG_ARRAY     <element count>  [marshalling ops for pointed-to type] FLAG_END
         * FLAG_CONTAINER <pointer-to-Container-object> [marshalling ops for pointed-to type] FLAG_END
         */
        enum Operations {
            FLAG_MEMCPY,
            FLAG_ARRAY,
            FLAG_CONTAINER,
            FLAG_SKIP,
            FLAG_END
        };

        enum InitOperations {
            FLAG_INIT,
            FLAG_INIT_REPEAT,
            FLAG_INIT_CONTAINER = FLAG_CONTAINER,
            FLAG_INIT_SKIP = FLAG_SKIP,
            FLAG_INIT_END  = FLAG_END
        };

        /** This visitor computes the memory layout for a given type. This memory
         * layout is used by quite a few memory operations
         *
         * You should not have to use this directly. Use
         * Typelib.layout_of(type) instead
         */
        class Visitor : public TypeVisitor
        {
        private:
            MemoryLayout& ops;
            bool   accept_pointers;
            bool   accept_opaques;

        protected:
            bool visit_ (Numeric const& type);
            bool visit_ (Enum    const& type);
            bool visit_ (Array   const& type);
            bool visit_ (Container const& type);
            bool visit_ (Compound const& type);
            bool visit_ (Pointer const& type);
            bool visit_ (OpaqueType const& type);

        public:
            Visitor(MemoryLayout& ops, bool accept_pointers = false, bool accept_opaques = false);
        };
    }

    inline MemoryLayout raw_layout_of(Type const& t, bool accept_pointers = false, bool accept_opaques = false)
    {
        MemoryLayout ret;
        MemLayout::Visitor visitor(ret, accept_pointers, accept_opaques);
        visitor.apply(t);
        return ret;
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
    inline MemoryLayout layout_of(Type const& t, bool accept_pointers = false, bool accept_opaques = false, bool merge_skip_copy = true, bool remove_trailing_skips = true)
    {
        MemoryLayout ret = raw_layout_of(t, accept_pointers, accept_opaques);
        if (remove_trailing_skips)
            ret.removeTrailingSkips();
        return ret.simplify(merge_skip_copy);
    }
}

#endif

