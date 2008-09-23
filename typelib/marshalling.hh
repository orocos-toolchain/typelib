#ifndef TYPELIB_MARSHALL_HH
#define TYPELIB_MARSHALL_HH

#include <typelib/typevisitor.hh>
#include <typelib/value.hh>
#include <boost/tuple/tuple.hpp>

namespace Typelib
{
    struct UnsupportedMarshalling : public std::runtime_error
    { 
        UnsupportedMarshalling(std::string const& what) : std::runtime_error("cannot marshal " + what) { }
    };

    struct UnknownMarshallingBytecode : public std::runtime_error
    { 
        UnknownMarshallingBytecode() : std::runtime_error("found an unknown marshalling bytecode operation") { }
    };
    struct MarshalledTypeMismatch : public std::runtime_error
    {
        MarshalledTypeMismatch() : std::runtime_error("provided type does not match the marshalled data") { }
    };

    typedef std::vector<size_t> MarshalOps;

    BOOST_STATIC_ASSERT((sizeof(size_t) == sizeof(void*)));
    /* This visitor swaps the endianness of the given value in-place */
    class MarshalOpsVisitor : public TypeVisitor
    {
    public:
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

    private:
        MarshalOps& ops;
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
        MarshalOpsVisitor(MarshalOps& ops);

        void apply(Type const& type);
        static std::pair<MarshalOps::const_iterator, size_t> dump(
                uint8_t* data, size_t in_offset,
                std::vector<uint8_t>& buffer,
                MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end);
        static boost::tuple<MarshalOps::const_iterator, size_t, size_t> load(
                uint8_t* data, size_t out_offset,
                std::vector<uint8_t> const& buffer, size_t in_offset,
                MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end);
    };

    std::vector<uint8_t> dump_to_memory(Value v);
    void dump_to_memory(Value v, std::vector<uint8_t>& buffer);
    void dump_to_memory(Value v, std::vector<uint8_t>& buffer, MarshalOps const& ops);

    void load_from_memory(Value v, std::vector<uint8_t> const& buffer);
    void load_from_memory(Value v, std::vector<uint8_t> const& buffer, MarshalOps const& ops);
}

#endif

