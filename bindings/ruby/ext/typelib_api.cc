#include <ruby.h>
extern "C" {
#include <dl.h>
}
#include <typelib/registry.hh>
#include <typelib/typevisitor.hh>
#include <typelib/value.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <utilmm/configfile/configset.hh>

using namespace Typelib;
using utilmm::config_set;
using std::string;

static VALUE mTypelib   = Qnil;
static VALUE cRegistry  = Qnil;

static VALUE cType      = Qnil;
static VALUE cPointer   = Qnil;
static VALUE cArray     = Qnil;
static VALUE cCompound  = Qnil;
static VALUE cEnum      = Qnil;

static VALUE cValue         = Qnil;
static VALUE cValueArray    = Qnil;
static VALUE cLibrary   = Qnil;

#include "tools.hh"
#include "type.hh"
#include "enum.hh"
#include "compound.hh"

#include "value.hh"
#include "visitors.hh"
#include "array.hh"
#include "ruby-dl.hh"

#include "registry.hh"

static
VALUE library_do_wrap(int argc, VALUE* argv, VALUE self)
{
    std::string dlspec = typelib_get_dl_spec(argc - 1, argv + 1);
    VALUE rb_dlspec = rb_str_new2(dlspec.c_str());
    return rb_funcall(self, rb_intern("[]"), 2, argv[0], rb_dlspec);
}

static
VALUE filter_immediate_arg(VALUE self, VALUE arg_val, VALUE rb_arg_type)
{
    Type const& arg_type = rb2cxx::object<Type>(rb_arg_type);

    if (arg_type.getCategory() == Type::Enum)
        return INT2FIX( rb2cxx::enum_value(arg_val, static_cast<Enum const&>(arg_type)) );
    else if (arg_type.getCategory() == Type::Pointer)
    {
        // Build directly a DL::Ptr object, no need to build a Ruby Value wrapper
        Pointer const& ptr_type = static_cast<Pointer const&>(arg_type);
        Type const& pointed_type = ptr_type.getIndirection();
        VALUE ptr = rb_dlptr_malloc(pointed_type.getSize(), free);

        Value typelib_value(rb_dlptr2cptr(ptr), pointed_type);
        if (!typelib_from_ruby(typelib_value, arg_val))
            return Qnil;

        return ptr;
    }
    return arg_val;
}

static 
VALUE filter_value_arg(VALUE self, VALUE arg_val, VALUE rb_arg_type)
{
    Type const& arg_type    = rb2cxx::object<Type>(rb_arg_type);
    Value const& value      = rb2cxx::object<Value>(arg_val);
    Type const& value_type  = value.getType();     

    if (value_type == arg_type)
    {
        if (value_type.getCategory() == Type::Pointer)
            return rb_dlptr_new(*reinterpret_cast<void**>(value.getData()), value_type.getSize(), do_not_delete);
        else if (value_type.getCategory() == Type::Numeric)
            return rb_funcall(arg_val, rb_intern("to_ruby"), 0);
        else
            return Qnil;
    }

    // There is only pointers left to handle
    if (arg_type.getCategory() != Type::Pointer)
        return Qnil;

    Pointer const& ptr_type   = static_cast<Pointer const&>(arg_type);
    Type const& pointed_type  = ptr_type.getIndirection();

    // /void == /nil, so that if expected_type is null, then 
    // it is because the argument can hold any kind of pointers
    if (pointed_type.isNull() || value_type == pointed_type)
        return rb_funcall(arg_val, rb_intern("to_ptr"), 0);
    
    // One thing left: array -> pointer convertion
    if (! value_type.getCategory() == Type::Array)
        return Qnil;

    Array const& array_type = static_cast<Array const&>(value_type);
    if (array_type.getIndirection() != pointed_type)
        return Qnil;
    return rb_funcall(arg_val, rb_intern("to_ptr"), 0);
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









/**********************************************
 * Typelib::Pointer
 */

static VALUE type_pointer_deference(VALUE self)
{
    VALUE registry = rb_iv_get(self, "@registry");
    Type const& type(rb2cxx::object<Type>(self));
    Pointer const& pointer(static_cast<Pointer const&>(type));
    return cxx2rb::type_wrap(pointer.getIndirection(), registry);
}









static void check_is_string_handler(VALUE object, char*& str, string::size_type& buffer_size)
{
    Value const& value(rb2cxx::object<Value>(object));
    Type  const& type(value.getType());
    if (type.getCategory() != Type::Array && type.getCategory() != Type::Pointer)
        rb_raise(rb_eTypeError, "Value#to_string called on a type which is neither char* nor char[]");

    Type const& data_type(static_cast<Indirect const&>(value.getType()).getIndirection());
    if (data_type.getName() != "/char")
        rb_raise(rb_eTypeError, "Value#to_string called on a type which is neither char* nor char[]");

    if (type.getCategory() == Type::Array)
    {
        // return the count of characters to consider
        str = reinterpret_cast<char*>(value.getData());
        buffer_size = static_cast<Array const&>(type).getDimension();
    }
    else
    {
        str = *reinterpret_cast<char**>(value.getData());
        buffer_size = string::npos;
    }
}
static string::size_type string_get_size(char const* str, string::size_type buffer_size)
{
    if (buffer_size == string::npos)
        return strlen(str);

    for (string::size_type i = 0; i < buffer_size; ++i)
        if (! str[i]) return i + 1;

    return buffer_size;
}



/*
 * Handle convertion between Ruby's String and C 'char*' types
 */
static VALUE value_from_string(VALUE self, VALUE from)
{
    char* str;
    string::size_type buffer_size;
    check_is_string_handler(self, str, buffer_size);

    if (buffer_size == string::npos)
        rb_raise(rb_eTypeError, "cannot initialize a char* pointer using a Ruby string. Use an array instead");

    string::size_type from_length = RSTRING(StringValue(from))->len;
    if ((buffer_size - 1) < from_length)
        rb_raise(rb_eTypeError, "array to small: %i, while %i was needed", buffer_size, from_length + 1);

    strncpy(str, StringValueCStr(from), buffer_size - 1);
    str[buffer_size - 1] = 0;
    return self;
}
static VALUE value_to_string(VALUE self)
{
    char* str;
    string::size_type buffer_size;
    check_is_string_handler(self, str, buffer_size);

    if (buffer_size == string::npos)
    {
        // TODO: maybe refuse doing this if the value object is tainted
        return rb_str_new2(str);
    }
    else
    {
        string::size_type string_size = string_get_size(str, buffer_size);
        return rb_str_new(str, string_size - 1);
    }
}







/**********************************************************************
 *
 * Extension of the standard libraries
 *
 */

static VALUE kernel_is_immediate(VALUE klass, VALUE object)
{ return IMMEDIATE_P(object) ? Qtrue : Qfalse; }
static VALUE dl_ptr_to_ptr(VALUE klass, VALUE ptr)
{
    VALUE newptr = rb_dlptr_malloc(sizeof(void*), free);
    *reinterpret_cast<void**>(rb_dlptr2cptr(newptr)) = rb_dlptr2cptr(ptr);
    // Protect ptr against GC
    rb_iv_set(newptr, "@points_on", newptr);
    return newptr;
}

extern "C" void Init_typelib_api()
{
    mTypelib  = rb_define_module("Typelib");
    rb_define_singleton_method(mTypelib, "do_call_function", RUBY_METHOD_FUNC(typelib_call_function), 4);
    rb_define_singleton_method(mTypelib, "filter_immediate_arg", RUBY_METHOD_FUNC(filter_immediate_arg), 2);
    rb_define_singleton_method(mTypelib, "filter_value_arg", RUBY_METHOD_FUNC(filter_value_arg), 2);
    
    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "do_get", RUBY_METHOD_FUNC(registry_do_get), 1);
    rb_define_method(cRegistry, "do_build", RUBY_METHOD_FUNC(registry_do_build), 1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 3);

    cType     = rb_define_class_under(mTypelib, "Type", rb_cObject);
    rb_define_alloc_func(cType, value_alloc);
    rb_define_singleton_method(cType, "name",  RUBY_METHOD_FUNC(type_name), 0);
    rb_define_singleton_method(cType, "null?", RUBY_METHOD_FUNC(type_is_null), 0);
    rb_define_singleton_method(cType, "==",    RUBY_METHOD_FUNC(type_equality), 1);
    rb_define_method(cType, "initialize",   RUBY_METHOD_FUNC(&value_initialize), 1);
    rb_define_method(cType, "to_string",    RUBY_METHOD_FUNC(&value_to_string), 0);
    rb_define_method(cType, "from_string",  RUBY_METHOD_FUNC(&value_from_string), 1);
    rb_define_method(cType, "to_ruby",      RUBY_METHOD_FUNC(&value_to_ruby), 0);

    cPointer  = rb_define_class_under(mTypelib, "PointerType", cType);
    rb_define_singleton_method(cPointer, "deference",    RUBY_METHOD_FUNC(type_pointer_deference), 0);
    rb_define_method(cPointer, "deference", RUBY_METHOD_FUNC(value_pointer_deference), 0);
    
    cArray    = rb_define_class_under(mTypelib, "ArrayType", cType);
    rb_define_method(cArray, "[]",      RUBY_METHOD_FUNC(array_get), 1);
    rb_define_method(cArray, "[]=",     RUBY_METHOD_FUNC(array_set), 2);
    rb_define_method(cArray, "each",    RUBY_METHOD_FUNC(array_each), 0);
    rb_define_method(cArray, "size",    RUBY_METHOD_FUNC(array_size), 0);
    
    cCompound = rb_define_class_under(mTypelib, "CompoundType", cType);
    rb_define_singleton_method(cCompound, "get_fields",   RUBY_METHOD_FUNC(compound_get_fields), 0);
// rb_define_singleton_method(cCompound, "writable?",  RUBY_METHOD_FUNC(compound_field_is_writable), 1);
    rb_define_method(cCompound, "get_field", RUBY_METHOD_FUNC(value_field_get), 1);
    rb_define_method(cCompound, "set_field", RUBY_METHOD_FUNC(value_field_set), 2);

    cEnum = rb_define_class_under(mTypelib, "EnumType", cType);
    rb_define_singleton_method(cEnum, "keys", RUBY_METHOD_FUNC(enum_keys), 0);

    cLibrary  = rb_const_get(mTypelib, rb_intern("Library"));
    // do_wrap arguments are formatted by Ruby code
    rb_define_method(cLibrary, "do_wrap", RUBY_METHOD_FUNC(library_do_wrap), -1);
    
    rb_define_method(rb_mKernel, "immediate?", RUBY_METHOD_FUNC(kernel_is_immediate), 1);
    rb_define_method(rb_cDLPtrData, "to_ptr", RUBY_METHOD_FUNC(dl_ptr_to_ptr), 0);
}



