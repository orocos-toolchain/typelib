#include <ruby.h>
extern "C" {
#include <dl.h>
}
#include <typelib/value.hh>
#include <typelib/registry.hh>
#include <typelib/typevisitor.hh>
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

#include "string.hh"
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








/**********************************************************************
 *
 * Extension of the standard libraries
 *
 */

static VALUE kernel_is_immediate(VALUE klass, VALUE object)
{ return IMMEDIATE_P(object) ? Qtrue : Qfalse; }
static VALUE dl_ptr_to_ptr(VALUE ptr)
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
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_do_get), 1);
    rb_define_method(cRegistry, "build", RUBY_METHOD_FUNC(registry_do_build), 1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 3);
    rb_define_method(cRegistry, "to_xml", RUBY_METHOD_FUNC(registry_to_xml), 0);
    rb_define_method(cRegistry, "alias", RUBY_METHOD_FUNC(registry_alias), 2);

    cType     = rb_define_class_under(mTypelib, "Type", rb_cObject);
    rb_define_alloc_func(cType, value_alloc);
    rb_define_singleton_method(cType, "==",    RUBY_METHOD_FUNC(type_equality), 1);
    rb_define_singleton_method(cType, "to_string",    RUBY_METHOD_FUNC(&value_to_string), 1);
    rb_define_singleton_method(cType, "from_string",  RUBY_METHOD_FUNC(&value_from_string), 2);
    rb_define_method(cType, "string_handler?", RUBY_METHOD_FUNC(&value_string_handler_p), 0);
    rb_define_method(cType, "__initialize__",   RUBY_METHOD_FUNC(&value_initialize), 1);
    rb_define_method(cType, "to_ruby",      RUBY_METHOD_FUNC(&value_to_ruby), 0);
    rb_define_method(cType, "zero!",      RUBY_METHOD_FUNC(&value_zero), 0);

    cPointer  = rb_define_class_under(mTypelib, "PointerType", cType);
    rb_define_singleton_method(cPointer, "deference",    RUBY_METHOD_FUNC(type_pointer_deference), 0);
    rb_define_method(cPointer, "deference", RUBY_METHOD_FUNC(value_pointer_deference), 0);
    rb_define_method(cPointer, "null?", RUBY_METHOD_FUNC(value_pointer_nil_p), 0);
    
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

