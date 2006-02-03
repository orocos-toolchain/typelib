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
static VALUE cValue     = Qnil;
static VALUE cRegistry  = Qnil;
static VALUE cType      = Qnil;
static VALUE cValueArray     = Qnil;
static VALUE cLibrary   = Qnil;

// NOP deleter, for Type objects and some Ptr objects
static void do_not_delete(void*) {}

/********************************
 *
 *  Define rb_get_cxx, to get the C++ object wrapped by a Ruby object
 *
 */
template<typename T> static VALUE rb_get_class();
template<> static VALUE rb_get_class<Registry>() { return cRegistry; }
template<> static VALUE rb_get_class<Type>()     { return cType; }
template<> static VALUE rb_get_class<Value>()    { return cValue; }

template<typename T>
static void check_is_kind_of(VALUE self)
{
    VALUE expected = rb_get_class<T>();
    if (! rb_obj_is_kind_of(self, expected))
        rb_raise(rb_eTypeError, "expected %s, got %s", rb_class2name(expected), rb_obj_classname(self));
}

template<typename T>
static T& rb_get_cxx(VALUE self)
{
    check_is_kind_of<T>(self);

    void* object = 0;
    Data_Get_Struct(self, void, object);
    return *reinterpret_cast<T*>(object);
}

template<typename T>
static VALUE rb_cxx_equality(VALUE rbself, VALUE rbwith)
{
    if (rb_obj_class(rbself) != rb_obj_class(rbwith))
        return Qfalse;

    T const& self(rb_get_cxx<T>(rbself));
    T const& with(rb_get_cxx<T>(rbwith));
    return (self == with) ? Qtrue : Qfalse;
}

static
VALUE typelib_wrap_type(Type const& type, VALUE registry)
{
    VALUE rtype = Data_Wrap_Struct(cType, 0, do_not_delete, const_cast<Type*>(&type));
    // Set a registry attribute to keep the registry alive
    rb_iv_set(rtype, "@registry", registry);
    return rtype;
}


#include "visitors.hh"
#include "array.hh"
#include "ruby-dl.hh"

/***********************************************************************************
 *
 * Wrapping of the Value class
 *
 */

static
void value_delete(void* self) { delete reinterpret_cast<Value*>(self); }

static
VALUE value_alloc(VALUE klass)
{ return Data_Wrap_Struct(klass, 0, value_delete, new Value); }

/** Initializes a Typelib::Value object using a Ruby::DL PtrData
 * object and a Typelib::Type object
 */
static
VALUE value_initialize(VALUE self, VALUE ptr, VALUE type)
{
    Value& value = rb_get_cxx<Value>(self);

    // Protect 'type' against the GC
    rb_iv_set(self, "@type", type);
    Type const& t(rb_get_cxx<Type>(type));

    if(NIL_P(ptr))
        ptr = rb_dlptr_malloc(t.getSize(), free);

    // Protect 'ptr' against the GC
    rb_iv_set(self, "@ptr", ptr);

    value = Value(rb_dlptr2cptr(ptr), t);
    
    return self;
}

/* Check if a given attribute exists and if it is writable
 *
 * Returns +nil+ if the attribute does not exist, 0 if it is readonly
 * and 1 if it is writable
 *
 * This method is not to be used by client code. It is called by respond_to?
 */
static
VALUE value_field_attributes(VALUE self, VALUE id)
{
    try {
        Value& v = rb_get_cxx<Value>(self);

        Type const& type(value_get_field(v, StringValuePtr(id)).getType());
        return type_is_assignable(type);
    } catch(FieldNotFound) { 
        return Qnil;
    }
}

static VALUE rbvalue_get_field(VALUE self, VALUE name)
{ return typelib_to_ruby(self, name); }
static VALUE rbvalue_set_field(VALUE self, VALUE name, VALUE newval)
{ return typelib_from_ruby(self, name, newval); }
static VALUE value_equality(VALUE rbself, VALUE rbwith)
{ return rb_cxx_equality<Value>(rbself, rbwith); }

/***********************************************************************************
 *
 * Wrapping of the Registry class
 *
 */


static 
void registry_free(void* ptr) { delete reinterpret_cast<Registry*>(ptr); }

static
VALUE registry_alloc(VALUE klass)
{
    Registry* registry = new Registry;
    return Data_Wrap_Struct(klass, 0, registry_free, registry);
}


static
VALUE registry_get(VALUE self, VALUE name)
{
    Registry& registry = rb_get_cxx<Registry>(self);
    Type const* type = registry.get( StringValuePtr(name) );

    if (! type) return Qnil;
    return typelib_wrap_type(*type, self);
}

static
VALUE registry_build(VALUE self, VALUE name)
{
    Registry& registry = rb_get_cxx<Registry>(self);
    Type const* type = registry.build( StringValuePtr(name) );

    if (! type) 
        rb_raise(rb_eTypeError, "invalid type %s", StringValuePtr(name));
    return typelib_wrap_type(*type, self);
}

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
    Type const& arg_type = rb_get_cxx<Type>(rb_arg_type);

    if (arg_type.getCategory() == Type::Enum)
        return INT2FIX( rb_enum_get_value(arg_val, static_cast<Enum const&>(arg_type)) );
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
    Type const& arg_type    = rb_get_cxx<Type>(rb_arg_type);
    Value const& value      = rb_get_cxx<Value>(arg_val);
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
        Type const& rettype(rb_get_cxx<Type>(return_type));
        if (rettype.getCategory() == Type::Enum)
        {
            Enum const& ret_enum(static_cast<Enum const&>(rettype));
            rb_ary_store(ret, 0, INT2FIX(enum_get_rb_symbol(ret, ret_enum)));
        }
    }

    return ret;
}

/* Private method to import a given file in the registry
 * We expect Registry#import to format the arguments before calling
 * do_import
 */
static
VALUE registry_import(VALUE self, VALUE file, VALUE kind, VALUE options)
{
    Registry& registry = rb_get_cxx<Registry>(self);
    
    PluginManager::self manager;
    Importer* importer = manager->importer( StringValuePtr(kind) );

    config_set config;
    if (! NIL_P(options))
    {
        for (int i = 0; i < RARRAY(options)->len; ++i)
        {
            VALUE entry = RARRAY(options)->ptr[i];
            VALUE k = RARRAY(entry)->ptr[0];
            VALUE v = RARRAY(entry)->ptr[1];

            if ( TYPE(v) == T_ARRAY )
            {
                for (int j = 0; j < RARRAY(v)->len; ++j)
                    config.insert(StringValuePtr(k), StringValuePtr( RARRAY(v)->ptr[j] ));
            }
            else
                config.set(StringValuePtr(k), StringValuePtr(v));
        }
    }
        
    // TODO: error checking
    if (!importer->load(StringValuePtr(file), config, registry))
        rb_raise(rb_eRuntimeError, "cannot import %s", StringValuePtr(file));

    return Qnil;
}

static VALUE type_is_a(VALUE self, Type::Category category)
{ 
    Type const& type(rb_get_cxx<Type>(self));
    return (type.getCategory() == category) ? Qtrue : Qfalse;
}
static VALUE type_is_array(VALUE self)      { return type_is_a(self, Type::Array); }
static VALUE type_is_compound(VALUE self)   { return type_is_a(self, Type::Compound); }
static VALUE type_is_pointer(VALUE self)    { return type_is_a(self, Type::Pointer); }
static VALUE type_is_null(VALUE self)    { return type_is_a(self, Type::NullType); }
static VALUE type_equality(VALUE rbself, VALUE rbwith)
{ return rb_cxx_equality<Type>(rbself, rbwith); }
Field const* type_get_cxx_field(VALUE self, VALUE field_name)
{
    Type const& type(rb_get_cxx<Type>(self));
    if (type.getCategory() != Type::Compound)
        rb_raise(rb_eTypeError, "expected a compound, got %s", rb_obj_classname(self));

    Compound const& compound = static_cast<Compound const&>(type);
    return compound.getField(StringValuePtr(field_name));
}
static VALUE type_has_field(VALUE self, VALUE field_name)
{ return type_get_cxx_field(self, field_name) ? Qtrue : Qfalse; }
static VALUE type_get_field(VALUE self, VALUE field_name)
{
    Field const* field = type_get_cxx_field(self, field_name);
    if (! field)
    {
        rb_raise(rb_eNoMethodError, "no such field '%s' in '%s'", 
                StringValuePtr(field_name), 
                rb_get_cxx<Type>(self).getName().c_str());
    }

    VALUE registry = rb_iv_get(self, "@registry");
    return typelib_wrap_type(field->getType(), registry);
}
static VALUE type_each_field(VALUE self)
{
    Type const& type(rb_get_cxx<Type>(self));
    if (type.getCategory() != Type::Compound)
        rb_raise(rb_eTypeError, "expected a compound, got %s", rb_obj_classname(self));

    VALUE registry = rb_iv_get(self, "@registry");

    Compound const& compound(static_cast<Compound const&>(type));
    Compound::FieldList const& fields(compound.getFields());
    Compound::FieldList::const_iterator it, end = fields.end();

    VALUE yield_ary = rb_ary_new2(2);
    for (it = fields.begin(); it != end; ++it)
    {
        VALUE field_name = rb_str_new2(it->getName().c_str());
        VALUE field_type = typelib_wrap_type(it->getType(), registry);
        rb_ary_store(yield_ary, 0, field_name);
        rb_ary_store(yield_ary, 1, field_type);
        rb_yield(yield_ary);
    }

    return Qnil;
}

static VALUE type_name(VALUE self)
{
    Type const& type = rb_get_cxx<Type>(self);
    return rb_str_new2(type.getName().c_str());
}
static Type const& type_pointer_deference_cxx(Type const& type)
{
    // This sucks. Must define type_deference on pointers only
    if (type.getCategory() != Type::Pointer)
        rb_raise(rb_eTypeError, "Type#deference called on a non-pointer type");

    Pointer const& pointer(static_cast<Pointer const&>(type));
    return pointer.getIndirection();
}

static VALUE value_pointer_deference(VALUE self)
{
    Value const& value(rb_get_cxx<Value>(self));
    Type  const& type(value.getType());
    Type  const& deference_type(type_pointer_deference_cxx(type));
    
    VALUE rb_type = rb_iv_get(self, "@type");
    VALUE registry = rb_iv_get(rb_type, "@registry");

    Value new_value( *reinterpret_cast<void**>(value.getData()), deference_type );
    return typelib_to_ruby(new_value, registry);
}

static void check_is_string_handler(VALUE object, char*& str, string::size_type& buffer_size)
{
    Value const& value(rb_get_cxx<Value>(object));
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

static VALUE type_pointer_deference(VALUE self)
{
    VALUE registry = rb_iv_get(self, "@registry");
    Type const& type(rb_get_cxx<Type>(self));
    Type const& deference_type = type_pointer_deference_cxx(type);
    return typelib_wrap_type(deference_type, registry);
}
static VALUE value_to_ruby(VALUE self)
{
    Value const& value(rb_get_cxx<Value>(self));
    VALUE type = rb_iv_get(self, "@type");
    VALUE registry = rb_iv_get(type, "@registry");
    return typelib_to_ruby(value, registry);
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
    
    cValue    = rb_define_class_under(mTypelib, "Value", rb_cObject);
    rb_define_alloc_func(cValue, value_alloc);
    rb_define_method(cValue, "initialize", RUBY_METHOD_FUNC(value_initialize), 2);
    rb_define_method(cValue, "field_attributes", RUBY_METHOD_FUNC(value_field_attributes), 1);
    rb_define_method(cValue, "get_field", RUBY_METHOD_FUNC(rbvalue_get_field), 1);
    rb_define_method(cValue, "set_field", RUBY_METHOD_FUNC(rbvalue_set_field), 2);
    rb_define_method(cValue, "deference", RUBY_METHOD_FUNC(value_pointer_deference), 0);
    rb_define_method(cValue, "to_ruby", RUBY_METHOD_FUNC(value_to_ruby), 0);
    rb_define_method(cValue, "==", RUBY_METHOD_FUNC(&value_equality), 1);
    rb_define_method(cValue, "to_string", RUBY_METHOD_FUNC(&value_to_string), 0);
    rb_define_method(cValue, "from_string", RUBY_METHOD_FUNC(&value_from_string), 1);

    cValueArray    = rb_define_class_under(mTypelib, "ValueArray", cValue);
    rb_define_alloc_func(cValueArray, value_alloc);
    // The initialize method is defined in the Ruby part of the library
    rb_define_method(cValueArray, "[]",      RUBY_METHOD_FUNC(typelib_array_get), 1);
    rb_define_method(cValueArray, "[]=",     RUBY_METHOD_FUNC(typelib_array_set), 2);
    rb_define_method(cValueArray, "each",    RUBY_METHOD_FUNC(typelib_array_each), 0);
    rb_define_method(cValueArray, "size",    RUBY_METHOD_FUNC(typelib_array_size), 0);

    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_get), 1);
    rb_define_method(cRegistry, "build", RUBY_METHOD_FUNC(registry_build), 1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 3);

    cType     = rb_define_class_under(mTypelib, "Type", rb_cObject);
    rb_define_method(cType, "array?",       RUBY_METHOD_FUNC(type_is_array), 0);
    rb_define_method(cType, "compound?",    RUBY_METHOD_FUNC(type_is_compound), 0);
    rb_define_method(cType, "pointer?",     RUBY_METHOD_FUNC(type_is_pointer), 0);
    rb_define_method(cType, "null?",        RUBY_METHOD_FUNC(type_is_null), 0);
    rb_define_method(cType, "deference",    RUBY_METHOD_FUNC(type_pointer_deference), 0);
    rb_define_method(cType, "==",           RUBY_METHOD_FUNC(&type_equality), 1);
    rb_define_method(cType, "has_field?",   RUBY_METHOD_FUNC(type_has_field), 1);
    rb_define_method(cType, "get_field",    RUBY_METHOD_FUNC(type_get_field), 1);
    rb_define_method(cType, "name",         RUBY_METHOD_FUNC(type_name), 0);
    rb_define_method(cType, "each_field",   RUBY_METHOD_FUNC(type_each_field), 0);

    cLibrary  = rb_const_get(mTypelib, rb_intern("Library"));
    // do_wrap arguments are formatted by Ruby code
    rb_define_method(cLibrary, "do_wrap", RUBY_METHOD_FUNC(library_do_wrap), -1);
    
    rb_define_method(rb_mKernel, "immediate?", RUBY_METHOD_FUNC(kernel_is_immediate), 1);
    rb_define_method(rb_cDLPtrData, "to_ptr", RUBY_METHOD_FUNC(dl_ptr_to_ptr), 0);
}



