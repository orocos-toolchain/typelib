#include "typelib.hh"
extern "C" {
#include <dl.h>
}

using namespace Typelib;

static VALUE cLibrary   = Qnil;

/*
 * Converts a Typelib::Type into its corresponding Ruby::DL spec
 *
 * Warnings:
 *   - Ruby::DL does not support long long
 *   - Because types in Ruby::DL are not of fixed sizes (think "char"),
 *     I cannot guarantee the behaviour of the Ruby::DL mapping
 */
class DLSpec : public TypeVisitor
{
    std::string m_spec;

    static char const* numeric_type_to_spec(Numeric const& type)
    {
        // Ruby/DL support for float and doubles on non-i386 machines is broken
        // see README
        if (type.getNumericCategory() != Numeric::Float)
        {

            switch(type.getSize())
            {
                case 1: return "C";
                case 2: return "H";
                case 4: return "L";
            }
        }

        throw UnsupportedType(type);
    }

    virtual bool visit_ (Numeric const& type)
    {
        m_spec = numeric_type_to_spec(type);
        return false;
    }
    virtual bool visit_ (Enum const& type)      
    { 
        BOOST_STATIC_ASSERT(( sizeof(Enum::integral_type) == sizeof(int) ));
        m_spec = "I"; 
        return false;
    }

    virtual bool visit_ (Pointer const& type)
    {
        m_spec = "P";
        return false;
    }
    virtual bool visit_ (Array const& type)
    { throw UnsupportedType(type); }

    virtual bool visit_ (Compound const& type)
    { throw UnsupportedType(type); }

public:

    // Returns the Ruby::DL spec for +type+
    // or throws UnsupportedType
    std::string apply(Type const& type)
    {
        m_spec = std::string();
        TypeVisitor::apply(type);
        if (m_spec.empty())
            throw UnsupportedType(type);
        return m_spec;
    }
};


/* Converts an array of wrapped Type objects into the corresponding Ruby::DL spec */
static std::string typelib_get_dl_spec(int argc, VALUE* argv)
{
    std::string spec;
    DLSpec converter;
    for (int i = 0; i < argc; ++i)
    {
        if (NIL_P(argv[i]))
            spec += "0";
        else
        {
            Type const& type(rb2cxx::object<Type>(argv[i]));
            spec += converter.apply(type);
        }
    }
    return spec;
}

static
VALUE filter_immediate_arg(VALUE self, VALUE arg, VALUE rb_expected_type)
{
    Type const& expected_type = rb2cxx::object<Type>(rb_expected_type);

    if (expected_type.getCategory() == Type::Enum)
        return INT2FIX( rb2cxx::enum_value(arg, static_cast<Enum const&>(expected_type)) );
    else if (expected_type.getCategory() == Type::Pointer)
    {
        // Build directly a DL::Ptr object, no need to build a Ruby Value wrapper
        Pointer const& ptr_type = static_cast<Pointer const&>(expected_type);
        Type const& pointed_type = ptr_type.getIndirection();
        VALUE ptr = rb_dlptr_malloc(pointed_type.getSize(), free);

        Value typelib_value(rb_dlptr2cptr(ptr), pointed_type);
        typelib_from_ruby(typelib_value, arg);
        return ptr;
    }
    return arg;
}

// NOP deleter, for Type objects and some Ptr objects
inline void do_not_delete(void*) {}

static 
VALUE filter_value_arg(VALUE self, VALUE arg, VALUE rb_expected_type)
{
    Type const& expected_type   = rb2cxx::object<Type>(rb_expected_type);
    Value const& arg_value      = rb2cxx::object<Value>(arg);
    Type const& arg_type        = arg_value.getType();     

    if (arg_type == expected_type)
    {
        if (arg_type.getCategory() == Type::Pointer)
            return rb_dlptr_new(*reinterpret_cast<void**>(arg_value.getData()), arg_type.getSize(), do_not_delete);
        else if (arg_type.getCategory() == Type::Numeric)
            return rb_funcall(arg, rb_intern("to_ruby"), 0);
        else
            return Qnil;
    }

    // There is only pointers left to handle
    if (expected_type.getCategory() != Type::Pointer)
        return Qnil;

    Pointer const& expected_ptr_type   = static_cast<Pointer const&>(expected_type);
    Type const& expected_pointed_type  = expected_ptr_type.getIndirection();

    // /void == /nil, so that if expected_type is null, then 
    // it is because the argument can hold any kind of pointers
    if (expected_pointed_type.isNull() || expected_pointed_type == arg_type)
        return rb_funcall(arg, rb_intern("to_dlptr"), 0);
    
    // One thing left: array -> pointer convertion
    if (! arg_type.getCategory() == Type::Array)
        return Qnil;

    Array const& array_type = static_cast<Array const&>(arg_type);
    if (array_type.getIndirection() != expected_pointed_type)
        return Qnil;
    return rb_funcall(arg, rb_intern("to_dlptr"), 0);
}

static
VALUE typelib_call_function(VALUE klass, VALUE wrapper, VALUE args, VALUE return_type, VALUE arg_types)
{
    Check_Type(args,  T_ARRAY);
    Check_Type(arg_types, T_ARRAY);

    // Call the function via DL and get the real return value
    VALUE ret = rb_funcall3(wrapper, rb_intern("call"), RARRAY(args)->len, RARRAY(args)->ptr);

    VALUE return_value = rb_ary_entry(ret, 0);
    if (!NIL_P(return_value))
    {
        Type const& rettype(rb2cxx::object<Type>(return_type));
        if (rettype.getCategory() == Type::Enum)
        {
            Enum const& ret_enum(static_cast<Enum const&>(rettype));
            rb_ary_store(ret, 0, cxx2rb::enum_symbol(ret, ret_enum));
        }
    }

    return ret;
}

static
VALUE library_do_wrap(int argc, VALUE* argv, VALUE self)
{
    std::string dlspec = typelib_get_dl_spec(argc - 1, argv + 1);
    VALUE rb_dlspec = rb_str_new2(dlspec.c_str());
    return rb_funcall(self, rb_intern("[]"), 2, argv[0], rb_dlspec);
}

void Typelib_init_functions()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    rb_define_singleton_method(mTypelib, "do_call_function", RUBY_METHOD_FUNC(typelib_call_function), 4);
    rb_define_singleton_method(mTypelib, "filter_immediate_arg", RUBY_METHOD_FUNC(filter_immediate_arg), 2);
    rb_define_singleton_method(mTypelib, "filter_value_arg", RUBY_METHOD_FUNC(filter_value_arg), 2);

    cLibrary  = rb_const_get(mTypelib, rb_intern("Library"));
    // do_wrap arguments are formatted by Ruby code
    rb_define_method(cLibrary, "do_wrap", RUBY_METHOD_FUNC(library_do_wrap), -1);
}

