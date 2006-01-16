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

// Never destroy a type. The Type objects are destroyed with
// the registry they belong
static void do_not_delete(void*) {}

static
Value* rb_value2cxx(VALUE self)
{
    Value* value = 0;
    Data_Get_Struct(self, Value, value);
    return value;
}
static
Registry* rb_registry2cxx(VALUE self)
{
    Registry* registry = 0;
    Data_Get_Struct(self, Registry, registry);
    return registry;
}

static
void value_delete(void* self) { delete reinterpret_cast<Value*>(self); }

static
VALUE value_alloc(VALUE klass)
{ return Data_Wrap_Struct(klass, 0, value_delete, new Value); }

static
VALUE typelib_wrap_type(Type const& type, VALUE registry)
{
    VALUE rtype = Data_Wrap_Struct(cType, 0, do_not_delete, const_cast<Type*>(&type));
    // Set a registry attribute to keep the registry alive
    rb_iv_set(rtype, "@registry", registry);
    return rtype;
}

/** This visitor takes a Value class and a field name,
 *  and returns the VALUE object which corresponds to
 *  the field, or returns nil
 */
class RubyGetter : public ValueVisitor
{
    VALUE m_value;
    VALUE m_self;

    virtual bool visit_ (int8_t  & value) { m_value = CHR2FIX(value); return false; }
    virtual bool visit_ (uint8_t & value) { m_value = CHR2FIX(value); return false; }
    virtual bool visit_ (int16_t & value) { m_value = INT2NUM(value); return false; }
    virtual bool visit_ (uint16_t& value) { m_value = INT2NUM(value); return false; }
    virtual bool visit_ (int32_t & value) { m_value = INT2NUM(value); return false; }
    virtual bool visit_ (uint32_t& value) { m_value = INT2NUM(value); return false; }
    virtual bool visit_ (int64_t & value) { m_value = LL2NUM(value);  return false; }
    virtual bool visit_ (uint64_t& value) { m_value = ULL2NUM(value); return false; }
    virtual bool visit_ (float   & value) { m_value = rb_float_new(value); return false; }
    virtual bool visit_ (double  & value) { m_value = rb_float_new(value); return false; }

    virtual bool visit_array    (Value const& v, Array const& a) { throw UnsupportedType(v.getType()); }
    virtual bool visit_compound (Value const& v, Compound const& c)
    { 
        VALUE ptr       = rb_dlptr_new(v.getData(), v.getType().getSize(), do_not_delete);
        VALUE self_type = rb_iv_get(m_self, "@type");
        VALUE new_type  = typelib_wrap_type(v.getType(), rb_iv_get(self_type, "@registry"));
        VALUE args[2] = { ptr, new_type };
        m_value = rb_class_new_instance(2, args, cValue);
        return false; 
    }
    virtual bool visit_enum     (Value const& v, Enum const& e)   { throw UnsupportedType(v.getType()); }
    
public:
    RubyGetter()
        : ValueVisitor(false) {}

    VALUE apply(VALUE self, VALUE name)
    {
        m_self = self;
        Value& value = *rb_value2cxx(self);

        m_value    = Qnil;
        try { 
            Value field_value = value_get_field(value, StringValuePtr(name));
            ValueVisitor::apply(field_value);
            return m_value;
        } 
        catch(FieldNotFound)    { return Qnil; }
        catch(UnsupportedType)  { return Qnil; }
    }
};

class RubySetter : public ValueVisitor
{
    VALUE m_value;

    virtual bool visit_ (int8_t  & value) { value = NUM2CHR(m_value); return false; }
    virtual bool visit_ (uint8_t & value) { value = NUM2CHR(m_value); return false; }
    virtual bool visit_ (int16_t & value) { value = NUM2LONG(m_value); return false; }
    virtual bool visit_ (uint16_t& value) { value = NUM2ULONG(m_value); return false; }
    virtual bool visit_ (int32_t & value) { value = NUM2LONG(m_value); return false; }
    virtual bool visit_ (uint32_t& value) { value = NUM2ULONG(m_value); return false; }
    virtual bool visit_ (int64_t & value) { value = NUM2LL(m_value);  return false; }
    virtual bool visit_ (uint64_t& value) { value = NUM2LL(m_value); return false; }
    virtual bool visit_ (float   & value) { value = NUM2DBL(m_value); return false; }
    virtual bool visit_ (double  & value) { value = NUM2DBL(m_value); return false; }

    virtual bool visit_array    (Value const& v, Array const&) { throw UnsupportedType(v.getType()); }
    virtual bool visit_compound (Value const& v, Compound const&) { throw UnsupportedType(v.getType()); }
    virtual bool visit_enum     (Value const& v, Enum const&) { throw UnsupportedType(v.getType()); }
    
public:
    RubySetter()
        : ValueVisitor(false) {}

    bool apply(VALUE self, VALUE name, VALUE new_value)
    {
        Value& v = *rb_value2cxx(self);

        m_value = new_value;
        try { 
            Value field_value = value_get_field(v, StringValuePtr(name));
            ValueVisitor::apply(field_value);
            return true;
        } 
        catch(FieldNotFound) { return false; } 
        catch(UnsupportedType) { return false; }
    }
};

static
VALUE value_initialize(VALUE self, VALUE ptr, VALUE type)
{
    Value* value = rb_value2cxx(self);

    // Protect 'type' against the GC
    rb_iv_set(self, "@type", type);
    Type const* t;
    Data_Get_Struct(type, Type, t);

    if(NIL_P(ptr))
        ptr = rb_dlptr_malloc(t->getSize(), free);

    // Protect 'ptr' against the GC
    rb_iv_set(self, "@ptr", ptr);

    *value = Value(rb_dlptr2cptr(ptr), *t);
    
    return self;
}

static VALUE type_is_assignable(Type const& type)
{
    switch(type.getCategory())
    {
    case Type::Numeric:
        return INT2FIX(1);
    case Type::Pointer:
        return type_is_assignable( dynamic_cast<Pointer const&>(type).getIndirection());
    default:
        return INT2FIX(0);
    }
    // never reached
}

static
VALUE value_field_attributes(VALUE self, VALUE id)
{
    try {
        Value* v = rb_value2cxx(self);

        Type const& type(value_get_field(*v, StringValuePtr(id)).getType());
        return type_is_assignable(type);
    } catch(FieldNotFound) { 
        return Qnil;
    }
}


/** value_get_field and value_set_field are called
 * from the method_missing method defined in Ruby
 */
static
VALUE rbvalue_get_field(VALUE self, VALUE name)
{
    RubyGetter getter;
    return getter.apply(self, name);
}
static
VALUE rbvalue_set_field(VALUE self, VALUE name, VALUE newval)
{
    RubySetter setter;
    if (!setter.apply(self, name, newval))
        return Qnil;
    return newval;
}

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
    Registry* registry = rb_registry2cxx(self);
    Type const* type = registry->get( StringValuePtr(name) );

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
    Registry* registry = rb_registry2cxx(self);
    
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
    importer->load(StringValuePtr(file), config, *registry);
    return Qnil;
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
    
    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_get), 1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 3);

    cType     = rb_define_class_under(mTypelib, "Type", rb_cObject);
}

