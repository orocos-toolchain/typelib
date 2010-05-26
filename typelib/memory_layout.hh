#ifndef TYPELIB_MEMORY_LAYOUT_HH
#define TYPELIB_MEMORY_LAYOUT_HH

#include <stdexcept>
#include <typelib/typevisitor.hh>
#include <vector>

namespace Typelib
{
    struct NoLayout : public std::runtime_error
    { 
        NoLayout(Type const& type, std::string const& reason)
            : std::runtime_error("there is no memory layout for type " + type.getName() + ": " + reason) { }
    };

    struct UnknownLayoutBytecode : public std::runtime_error
    { 
        UnknownLayoutBytecode() : std::runtime_error("found an unknown marshalling bytecode operation") { }
    };

    // This is needed because the FLAG_CONTAINER descriptor is followed by a
    // pointer on a Container instance
    BOOST_STATIC_ASSERT((sizeof(size_t) == sizeof(void*)));

    typedef std::vector<size_t> MemoryLayout;

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

        /* This visitor computes the memory layout for a given type. This memory
         * layout is used by quite a few memory operations
         */
        class Visitor : public TypeVisitor
        {
        private:
            MemoryLayout& ops;
            bool   accept_pointers;
            bool   accept_opaques;
            size_t current_memcpy;

        protected:
            void push_current_memcpy();
            void skip(size_t count);
            bool generic_visit(Type const& value);
            bool visit_ (Numeric const& type);
            bool visit_ (Enum    const& type);
            bool visit_ (Array   const& type);
            bool visit_ (Container const& type);
            bool visit_ (Compound const& type);
            bool visit_ (Pointer const& type);
            bool visit_ (OpaqueType const& type);

        public:
            Visitor(MemoryLayout& ops, bool accept_pointers = false, bool accept_opaques = false);

            void apply(Type const& type);
        };

        /** Returns the iterator on the next FLAG_END, taking into account nesting */
        MemoryLayout::const_iterator skip_block(
                MemoryLayout::const_iterator begin,
                MemoryLayout::const_iterator end);
    }

    inline MemoryLayout layout_of(Type const& t, bool accept_pointers = false, bool accept_opaques = false)
    {
        MemoryLayout ret;
        MemLayout::Visitor visitor(ret, accept_pointers, accept_opaques);
        visitor.apply(t);
        return ret;
    }
}

#endif

