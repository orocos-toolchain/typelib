namespace cxx2rb {
    /* There are constraints when creating a Ruby wrapper for a Type,
     * mainly for avoiding GC issues
     * This function does the work
     * It needs the registry v type is from
     */
    static VALUE value_wrap(Value v, VALUE registry, VALUE klass)
    {
        VALUE type      = type_wrap(v.getType(), registry);
        VALUE ptr       = rb_dlptr_new(v.getData(), v.getType().getSize(), do_not_delete);
        return rb_funcall(type, rb_intern("new"), 1, ptr);
    }
}

/***********************************************************************************
 *
 * Wrapping of the Value class
 *
 */

static void value_delete(void* self) { delete reinterpret_cast<Value*>(self); }

static VALUE value_alloc(VALUE klass)
{ return Data_Wrap_Struct(klass, 0, value_delete, new Value); }

/** Initializes a Typelib::Value object using a Ruby::DL PtrData
 * object and a Typelib::Type object
 */
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

static
VALUE value_get_registry(VALUE self)
{
    VALUE type = rb_class_of(self);
    return rb_iv_get(type, "@registry");
}

static VALUE value_field_get(VALUE value, VALUE name)
{ 
    // Get the registry
    VALUE registry = value_get_registry(value);
    return typelib_to_ruby(rb2cxx::object<Value>(value), name, registry); 
}
static VALUE value_field_set(VALUE self, VALUE name, VALUE newval)
{ 
    Value& tlib_value(rb2cxx::object<Value>(self));
    try { 
        Value field_value = value_get_field(tlib_value, StringValuePtr(name));
        typelib_from_ruby(field_value, newval);
        return newval;
    } 
    catch(FieldNotFound)   { return Qnil; } 
}
static VALUE value_to_ruby(VALUE self)
{
    Value const& value(rb2cxx::object<Value>(self));
    VALUE type = rb_iv_get(self, "@type");
    VALUE registry = rb_iv_get(type, "@registry");
    return typelib_to_ruby(value, registry);
}

static VALUE value_pointer_deference(VALUE self)
{
    Value const& value(rb2cxx::object<Value>(self));
    Indirect const& indirect(static_cast<Indirect const&>(value.getType()));
    
    VALUE rb_type  = rb_iv_get(self, "@type");
    VALUE registry = rb_iv_get(rb_type, "@registry");

    Value new_value( *reinterpret_cast<void**>(value.getData()), indirect.getIndirection() );
    return typelib_to_ruby(new_value, registry);
}

