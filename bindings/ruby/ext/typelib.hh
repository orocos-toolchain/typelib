#ifndef __RUBY_EXT_TYPELIB_HH__
#define __RUBY_EXT_TYPELIB_HH__

#include <ruby.h>
#include <typelib/typemodel.hh>
#include <typelib/value.hh>
#include <typelib/registry.hh>

#include "typelib_ruby.hh"

#undef VERBOSE

namespace typelib_ruby {
    extern VALUE cType;
    extern VALUE cIndirect;
    extern VALUE cPointer;
    extern VALUE cArray;
    extern VALUE cCompound;
    extern VALUE cNumeric;
    extern VALUE cEnum;
    extern VALUE cContainer;
    extern VALUE cOpaque;
    extern VALUE cNull;
    extern VALUE cRegistry;

    extern VALUE eNotFound;

    /** Initialization routines */
    extern void Typelib_init_memory();
    extern void Typelib_init_values();
    extern void Typelib_init_strings();
    extern void Typelib_init_specialized_types();
    extern void Typelib_init_registry();
#ifdef WITH_DYNCALL
    extern void Typelib_init_functions();
#endif

    namespace cxx2rb {
        using namespace Typelib;

        typedef std::map<Type const*, std::pair<bool, VALUE> > WrapperMap;

        struct RbRegistry
        {
            boost::shared_ptr<Typelib::Registry> registry;
            /** Map that stores the mapping from Type instances to the
             * corresponding Ruby object
             *
             * The boolean flag is false if the registry still owns that type,
             * and true otherwise. The registry loses ownership of a type
             * through the remove call
             *
             * In both cases, they will be deleted only when the registry is
             * garbage collected.
             */
            cxx2rb::WrapperMap wrappers;

            RbRegistry(Typelib::Registry* registry)
                : registry(registry) {}
            ~RbRegistry() {}
        };

        VALUE class_of(Type const& type);

        template<typename T> VALUE class_of();
        template<> inline VALUE class_of<Value>()    { return cType; }
        template<> inline VALUE class_of<Type>()     { return rb_cClass; }
        template<> inline VALUE class_of<RbRegistry>() { return cRegistry; }

        VALUE type_wrap(Type const& type, VALUE registry);

        /* Get the Ruby symbol associated with a C enum, or nil
         * if the value is not valid for this enum
         */
        inline VALUE enum_symbol(Enum::integral_type value, Enum const& e)
        {
            try { 
                std::string symbol = e.get(value);
                return ID2SYM(rb_intern(symbol.c_str())); 
            }
            catch(Enum::ValueNotFound) { return Qnil; }
        }

        VALUE value_wrap(Value v, VALUE registry, VALUE parent = Qnil);
    }

    namespace rb2cxx {
        using namespace Typelib;

        inline void check_is_kind_of(VALUE self, VALUE expected)
        {
            if (! rb_obj_is_kind_of(self, expected))
                rb_raise(rb_eTypeError, "expected %s, got %s", rb_class2name(expected), rb_obj_classname(self));
        }

        template<typename T>
        T& get_wrapped(VALUE self)
        {
            void* object = 0;
            Data_Get_Struct(self, void, object);
            return *reinterpret_cast<T*>(object);
        }

        template<typename T> 
        T& object(VALUE self)
        {
            check_is_kind_of(self, cxx2rb::class_of<T>());
            return get_wrapped<T>(self);
        }

        template<> inline Registry& object(VALUE self)
        {
            return *object<cxx2rb::RbRegistry>(self).registry;
        }

        template<>
        inline Type& object(VALUE self) 
        {
            check_is_kind_of(self, rb_cClass);
            VALUE type = rb_iv_get(self, "@type");
            return get_wrapped<Type>(type);
        }

        Enum::integral_type enum_value(VALUE rb_value, Enum const& e);
    }

    class RubyGetter : public Typelib::ValueVisitor
    {
    protected:
        VALUE m_value;
        VALUE m_registry;
        VALUE m_parent;

        bool visit_ (int8_t  & value);
        bool visit_ (uint8_t & value);
        bool visit_ (int16_t & value);
        bool visit_ (uint16_t& value);
        bool visit_ (int32_t & value);
        bool visit_ (uint32_t& value);
        bool visit_ (int64_t & value);
        bool visit_ (uint64_t& value);
        bool visit_ (float   & value);
        bool visit_ (double  & value);

        bool visit_(Typelib::Value const& v, Typelib::Pointer const& p);
        bool visit_(Typelib::Value const& v, Typelib::Array const& a);
        bool visit_(Typelib::Value const& v, Typelib::Compound const& c);
        bool visit_(Typelib::Value const& v, Typelib::Container const& c);
        bool visit_(Typelib::Value const& v, Typelib::OpaqueType const& c);
        bool visit_(Typelib::Enum::integral_type& v, Typelib::Enum const& e);
        
    public:
        RubyGetter();
        ~RubyGetter();

        VALUE apply(Typelib::Value value, VALUE registry, VALUE parent);
    };

    class RubySetter : public Typelib::ValueVisitor
    {
    protected:
        VALUE m_value;

        bool visit_ (int8_t  & value);
        bool visit_ (uint8_t & value);
        bool visit_ (int16_t & value);
        bool visit_ (uint16_t& value);
        bool visit_ (int32_t & value);
        bool visit_ (uint32_t& value);
        bool visit_ (int64_t & value);
        bool visit_ (uint64_t& value);
        bool visit_ (float   & value);
        bool visit_ (double  & value);

        bool visit_(Typelib::Value const& v, Typelib::Pointer const& p);
        bool visit_(Typelib::Value const& v, Typelib::Array const& a);
        bool visit_(Typelib::Value const& v, Typelib::Compound const& c);
        bool visit_(Typelib::Value const& v, Typelib::Container const& c);
        bool visit_(Typelib::Value const& v, Typelib::OpaqueType const& c);
        bool visit_(Typelib::Enum::integral_type& v, Typelib::Enum const& e);
        
    public:
        RubySetter();
        ~RubySetter();

        VALUE apply(Typelib::Value value, VALUE new_value);
    };

    extern VALUE value_get_registry(VALUE self);
    extern VALUE type_get_registry(VALUE self);
    extern VALUE memory_wrap(void* ptr, bool take_ownership = false);
    extern VALUE memory_allocate(size_t size);
    extern void  memory_init(VALUE ptr, VALUE type);
    extern void* memory_cptr(VALUE ptr);
}

#endif

