#ifndef __RUBY_EXT_TYPELIB_HH__
#define __RUBY_EXT_TYPELIB_HH__

#include <ruby.h>
#include <typelib/typemodel.hh>
#include <typelib/value.hh>
#include <typelib/registry.hh>

#include "typelib_ruby.hh"

extern VALUE cType;
extern VALUE cIndirect;
extern VALUE cPointer;
extern VALUE cArray;
extern VALUE cCompound;
extern VALUE cEnum;

/** Initialization routines */
extern void Typelib_init_values();
extern void Typelib_init_strings();
extern void Typelib_init_functions();
extern void Typelib_init_specialized_types();
extern void Typelib_init_registry();

namespace cxx2rb {
    using namespace Typelib;

    VALUE class_of(Type const& type);

    template<typename T> VALUE class_of();
    template<> inline VALUE class_of<Value>()    { return cType; }
    template<> inline VALUE class_of<Type>()     { return rb_cClass; }

    typedef std::map<Type const*, VALUE> WrapperMap;
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

    VALUE value_wrap(Value v, VALUE registry, VALUE klass, VALUE dlptr);
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

    template<>
    inline Type& object(VALUE self) 
    {
        check_is_kind_of(self, rb_cClass);
        VALUE type = rb_iv_get(self, "@type");
        return get_wrapped<Type>(type);
    }

    Enum::integral_type enum_value(VALUE rb_value, Enum const& e);
}

VALUE value_get_registry(VALUE self);

#endif

