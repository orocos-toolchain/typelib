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
static VALUE cArray     = Qnil;

// NOP deleter, for Type objects and some Ptr objects
static void do_not_delete(void*) {}

static Registry& rb_registry2cxx(VALUE self)
{
    Registry* registry = 0;
    Data_Get_Struct(self, Registry, registry);
    return *registry;
}

static
VALUE typelib_wrap_type(Type const& type, VALUE registry)
{
    VALUE rtype = Data_Wrap_Struct(cType, 0, do_not_delete, const_cast<Type*>(&type));
    // Set a registry attribute to keep the registry alive
    rb_iv_set(rtype, "@registry", registry);
    return rtype;
}


static Value& rb_value2cxx(VALUE self)
{
    Value* value = 0;
    Data_Get_Struct(self, Value, value);
    return *value;
}

#include "visitors.hh"
#include "array.hh"

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
    Value& value = rb_value2cxx(self);

    // Protect 'type' against the GC
    rb_iv_set(self, "@type", type);
    Type const* t;
    Data_Get_Struct(type, Type, t);

    if(NIL_P(ptr))
        ptr = rb_dlptr_malloc(t->getSize(), free);

    // Protect 'ptr' against the GC
    rb_iv_set(self, "@ptr", ptr);

    value = Value(rb_dlptr2cptr(ptr), *t);
    
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
        Value& v = rb_value2cxx(self);

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
    Registry& registry = rb_registry2cxx(self);
    Type const* type = registry.get( StringValuePtr(name) );

    if (! type) return Qnil;
    return typelib_wrap_type(*type, self);
}

/* Private method to import a given file in the registry
 * We expect Registry#import to format the arguments before calling
 * do_import
 */
static
VALUE registry_import(VALUE self, VALUE file, VALUE kind, VALUE options)
{
    Registry& registry = rb_registry2cxx(self);
    
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
    importer->load(StringValuePtr(file), config, registry);
    return Qnil;
}

static VALUE type_is_array(VALUE self)
{ 
    Type const* type;
    Data_Get_Struct(self, Type, type);
    return (type->getCategory() == Type::Array) ? Qtrue : Qfalse;
}

extern "C" void Init_typelib_api()
{
    mTypelib  = rb_define_module("Typelib");
    
    cValue    = rb_define_class_under(mTypelib, "Value", rb_cObject);
    rb_define_alloc_func(cValue, value_alloc);
    rb_define_method(cValue, "initialize", RUBY_METHOD_FUNC(value_initialize), 2);
    rb_define_method(cValue, "field_attributes", RUBY_METHOD_FUNC(value_field_attributes), 1);
    rb_define_method(cValue, "get_field", RUBY_METHOD_FUNC(rbvalue_get_field), 1);
    rb_define_method(cValue, "set_field", RUBY_METHOD_FUNC(rbvalue_set_field), 2);

    cArray    = rb_define_class_under(mTypelib, "Array", cValue);
    rb_define_alloc_func(cArray, value_alloc);
    // The initialize method is defined in the Ruby part of the library
    rb_define_method(cArray, "[]",      RUBY_METHOD_FUNC(typelib_array_get), 1);
    rb_define_method(cArray, "[]=",     RUBY_METHOD_FUNC(typelib_array_set), 2);
    rb_define_method(cArray, "each",    RUBY_METHOD_FUNC(typelib_array_each), 0);
    rb_define_method(cArray, "size",    RUBY_METHOD_FUNC(typelib_array_size), 0);

    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_get), 1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 3);

    cType     = rb_define_class_under(mTypelib, "Type", rb_cObject);
    rb_define_method(cType, "array?", RUBY_METHOD_FUNC(type_is_array), 0);
}


