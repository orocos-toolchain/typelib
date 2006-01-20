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

    if (! type) return Qnil;
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
VALUE typelib_call_function(VALUE klass, VALUE wrapper, VALUE args, VALUE return_type, VALUE arg_types)
{
    Check_Type(args,  T_ARRAY);
    Check_Type(arg_types, T_ARRAY);

    // Get the arguments
    size_t argcount = RARRAY(args)->len;
    VALUE new_args = rb_ary_new2(argcount);
    for (size_t i = 0; i < argcount; ++i)
    {
        VALUE object = RARRAY(args)->ptr[i];
        // first type is the return type
        Type const& type = rb_get_cxx<Type>(RARRAY(arg_types)->ptr[i]);

        // Manage immediate values
        if (IMMEDIATE_P(object))
        {
            if (type.getCategory() == Type::Enum)
                object = INT2FIX( rb_enum_get_value(object, dynamic_cast<Enum const&>(type)) );
            else if (type.getCategory() == Type::Pointer)
            {
                // Build directly a DL::Ptr object, no need to build a Ruby Value wrapper
                // TODO: typechecking
                Pointer const& ptr_type = static_cast<Pointer const&>(type);
                Type const& pointed_type = ptr_type.getIndirection();
                VALUE ptr = rb_dlptr_malloc(pointed_type.getSize(), free);

                Value typelib_value(rb_dlptr2cptr(ptr), pointed_type);
                if (!typelib_from_ruby(typelib_value, object))
                    rb_raise(rb_eTypeError, "Wrong argument type %s", rb_obj_classname(object));

                object = ptr;
            }
        }

        if (type.getCategory() == Type::Pointer 
                && !rb_obj_is_kind_of(object, rb_cDLPtrData))
        {
            Pointer const& ptr_type     = static_cast<Pointer const&>(type);
            Type const& expected_type   = ptr_type.getIndirection();
            Type const& object_type     = rb_get_cxx<Value>(object).getType();

            // /void == /nil, so that if expected_type is null, then 
            // it ptr_type can hold anything
            if (!expected_type.isNull() && object_type != expected_type)
                rb_raise(rb_eTypeError, "expected %s, got %s", 
                        ptr_type.getIndirection().getName().c_str(),
                        object_type.getName().c_str());

            object = rb_funcall(object, rb_intern("to_ptr"), 0);
        }

        RARRAY(new_args)->ptr[i] = object;
    }

    // Call the function via DL and get the real return value
    VALUE ret = rb_funcall3(wrapper, rb_intern("call"), argcount, RARRAY(new_args)->ptr);
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

extern "C" void Init_typelib_api()
{
    mTypelib  = rb_define_module("Typelib");
    rb_define_singleton_method(mTypelib, "do_call_function", RUBY_METHOD_FUNC(typelib_call_function), 4);
    
    cValue    = rb_define_class_under(mTypelib, "Value", rb_cObject);
    rb_define_alloc_func(cValue, value_alloc);
    rb_define_method(cValue, "initialize", RUBY_METHOD_FUNC(value_initialize), 2);
    rb_define_method(cValue, "field_attributes", RUBY_METHOD_FUNC(value_field_attributes), 1);
    rb_define_method(cValue, "get_field", RUBY_METHOD_FUNC(rbvalue_get_field), 1);
    rb_define_method(cValue, "set_field", RUBY_METHOD_FUNC(rbvalue_set_field), 2);
    rb_define_method(cValue, "deference", RUBY_METHOD_FUNC(value_pointer_deference), 0);
    rb_define_method(cValue, "to_ruby", RUBY_METHOD_FUNC(value_to_ruby), 0);
    rb_define_method(cValue, "==", RUBY_METHOD_FUNC(&value_equality), 1);

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
    rb_define_method(cType, "==", RUBY_METHOD_FUNC(&type_equality), 1);
    rb_define_method(cType, "has_field?",   RUBY_METHOD_FUNC(type_has_field), 1);
    rb_define_method(cType, "get_field",    RUBY_METHOD_FUNC(type_get_field), 1);

    cLibrary  = rb_const_get(mTypelib, rb_intern("Library"));
    // do_wrap arguments are formatted by Ruby code
    rb_define_method(cLibrary, "do_wrap", RUBY_METHOD_FUNC(library_do_wrap), -1);
}


