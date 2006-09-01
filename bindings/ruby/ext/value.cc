#include "typelib.hh"
extern "C" {
#include <dl.h>
}

#include <typelib/value.hh>
#include <typelib/registry.hh>
#include <typelib/typevisitor.hh>

using namespace Typelib;

// NOP deleter, for Type objects and some Ptr objects
static void do_not_delete(void*) {}

namespace cxx2rb {
    /* There are constraints when creating a Ruby wrapper for a Type,
     * mainly for avoiding GC issues
     * This function does the work
     * It needs the registry v type is from
     */
    VALUE value_wrap(Value v, VALUE registry, VALUE klass)
    {
        VALUE type      = type_wrap(v.getType(), registry);
        VALUE ptr       = rb_dlptr_new(v.getData(), v.getType().getSize(), do_not_delete);
        return rb_funcall(type, rb_intern("wrap"), 1, ptr);
    }
}

VALUE cType	 = Qnil;
VALUE cPointer   = Qnil;
VALUE cArray     = Qnil;
VALUE cCompound  = Qnil;
VALUE cEnum      = Qnil;

VALUE cxx2rb::class_of(Typelib::Type const& type)
{
    using Typelib::Type;
    switch(type.getCategory()) {
	case Type::Compound:    return cCompound;
	case Type::Pointer:     return cPointer;
	case Type::Array:       return cArray;
	case Type::Enum:        return cEnum;
	default:                return cType;
    }
}

VALUE cxx2rb::type_wrap(Type const& type, VALUE registry)
{
    VALUE known_types = rb_iv_get(registry, "@wrappers");
    if (NIL_P(known_types))
	rb_raise(rb_eArgError, "@wrappers is uninitialized");

    // Type objects are unique, we can register Ruby wrappers based
    // on the type pointer (instead of using names)
    WrapperMap& wrappers = rb2cxx::get_wrapped<WrapperMap>(known_types);

    WrapperMap::const_iterator it = wrappers.find(&type);
    if (it != wrappers.end())
	return it->second;

    VALUE base  = class_of(type);
    VALUE klass = rb_class_new_instance(1, &base, rb_cClass);
    VALUE rb_type = Data_Wrap_Struct(rb_cObject, 0, do_not_delete, const_cast<Type*>(&type));
    rb_iv_set(klass, "@registry", registry);
    rb_iv_set(klass, "@type", rb_type);
    rb_iv_set(klass, "@name", rb_str_new2(type.getName().c_str()));
    rb_iv_set(klass, "@null", (type.getCategory() == Type::NullType) ? Qtrue : Qfalse);

    if (rb_respond_to(klass, rb_intern("subclass_initialize")))
	rb_funcall(klass, rb_intern("subclass_initialize"), 0);

    wrappers.insert(std::make_pair(&type, klass));
    return klass;
}

/**********************************************
 * Typelib::Type
 */

static VALUE type_equal_operator(VALUE rbself, VALUE rbwith)
{ 
    if (rb_obj_class(rbself) != rb_obj_class(rbwith))
        return Qfalse;

    Type const& self(rb2cxx::object<Type>(rbself));
    Type const& with(rb2cxx::object<Type>(rbwith));
    bool result = (self == with) || self.isSame(with);
    return result ? Qtrue : Qfalse;
}


/* PODs are assignable, pointers are dereferenced */
static VALUE type_is_assignable(Type const& type)
{
    switch(type.getCategory())
    {
    case Type::Numeric:
        return INT2FIX(1);
    case Type::Pointer:
        return type_is_assignable( dynamic_cast<Pointer const&>(type).getIndirection());
    case Type::Enum:
        return INT2FIX(1);
    default:
        return INT2FIX(0);
    }
    // never reached
}


/***********************************************************************************
 *
 * Wrapping of the Value class
 *
 */

static void value_delete(void* self) { delete reinterpret_cast<Value*>(self); }

static VALUE value_alloc(VALUE klass)
{ return Data_Wrap_Struct(klass, 0, value_delete, new Value); }

static
VALUE value_initialize(VALUE self, VALUE ptr)
{
    Value& value = rb2cxx::object<Value>(self);
    Type const& t(rb2cxx::object<Type>(rb_class_of(self)));

    if(NIL_P(ptr))
        ptr = rb_dlptr_malloc(t.getSize(), free);

    // Protect 'ptr' against the GC
    rb_iv_set(self, "@ptr", ptr);

    value = Value(rb_dlptr2cptr(ptr), t);
    return self;
}

VALUE value_memory_eql_p(VALUE rbself, VALUE rbwith)
{
    Value& self = rb2cxx::object<Value>(rbself);
    Value& with = rb2cxx::object<Value>(rbwith);
    if (self.getData() == with.getData())
	return Qtrue;
    
    // Type#== checks for type equality before calling memory_equal?
    Type const& type = self.getType();
    return memcmp(self.getData(), with.getData(), type.getSize()) == 0 ? Qtrue : Qfalse;
}

VALUE value_get_registry(VALUE self)
{
    VALUE type = rb_class_of(self);
    return rb_iv_get(type, "@registry");
}

/* Converts +self+ to its Ruby equivalent. If no equivalent
 * type is available, returns self
 */
static VALUE value_to_ruby(VALUE self)
{
    Value const& value(rb2cxx::object<Value>(self));
    VALUE registry = value_get_registry(self);
    return typelib_to_ruby(value, registry);
}

/* Initializes the memory to 0 */
static VALUE value_zero(VALUE self)
{
    Value const& value(rb2cxx::object<Value>(self));
    memset(value.getData(), value.getType().getSize(), 0);
    return self;
}


void Typelib_init_values(VALUE mTypelib)
{
    cType     = rb_define_class_under(mTypelib, "Type", rb_cObject);
    rb_define_alloc_func(cType, value_alloc);
    rb_define_singleton_method(cType, "==",	    RUBY_METHOD_FUNC(type_equal_operator), 1);
    rb_define_method(cType, "__initialize__",   RUBY_METHOD_FUNC(&value_initialize), 1);
    rb_define_method(cType, "to_ruby",      RUBY_METHOD_FUNC(&value_to_ruby), 0);
    rb_define_method(cType, "zero!",      RUBY_METHOD_FUNC(&value_zero), 0);
    rb_define_method(cType, "memory_eql?",      RUBY_METHOD_FUNC(&value_memory_eql_p), 1);

    Typelib_init_specialized_types(mTypelib);
}

