static VALUE typelib_value_get_registry(VALUE v)
{
    VALUE v_type = rb_iv_get(v, "@type");
    return rb_iv_get(v_type, "@registry");
}
static VALUE typelib_to_ruby(Value v, VALUE registry);
static VALUE typelib_to_ruby(Value value, VALUE name, VALUE registry);
static VALUE typelib_to_ruby(VALUE value, VALUE name);
static VALUE typelib_from_ruby(Value value, VALUE new_value);
static VALUE typelib_from_ruby(VALUE value, VALUE name, VALUE new_value);

/* Get the Ruby symbol associated with a C enum, or nil
 * if the value is not valid for this enum
 */
VALUE enum_get_rb_symbol(Enum::integral_type value, Enum const& e)
{
    std::string symbol = e.get(value);
    try { return ID2SYM(rb_intern(symbol.c_str())); }
    catch(Enum::ValueNotFound) { return Qnil; }
}

/* Get the C value associated with a ruby representation of an enum
 * Valid ruby representations are: symbols, strings and integer
 */
Enum::integral_type rb_enum_get_value(VALUE rb_value, Enum const& e)
{
    // m_value can be either an integer, a symbol or a string
    if (TYPE(rb_value) == T_FIXNUM)
    {
        Enum::integral_type value = FIX2INT(rb_value);
        try { e.get(value); }
        catch(Enum::ValueNotFound)
        { rb_raise(rb_eArgError, "%i is not a valid value for %s", value, e.getName().c_str()); }

        return value;
    }

    char const* name;
    if (SYMBOL_P(rb_value))
        name = rb_id2name(SYM2ID(rb_value));
    else
        name = StringValuePtr(rb_value);

    try { return e.get(name); }
    catch(Enum::SymbolNotFound)
    { rb_raise(rb_eArgError, "%s is not a valid symbol for %s", name, e.getName().c_str()); }
}

/* There are constraints when creating a Ruby wrapper for a Type,
 * mainly for avoiding GC issues
 * This function does the work
 * It needs the registry v type is from
 */
static VALUE wrap_value(Value v, VALUE registry, VALUE klass)
{
    VALUE new_type  = typelib_wrap_type(v.getType(), registry);
    VALUE ptr       = rb_dlptr_new(v.getData(), v.getType().getSize(), do_not_delete);
    VALUE args[2] = { ptr, new_type };
    return rb_class_new_instance(2, args, klass);
}

/* This visitor takes a Value class and a field name,
 * and returns the VALUE object which corresponds to
 * the field, or returns nil
 */
class RubyGetter : public ValueVisitor
{
    VALUE m_value;
    VALUE m_registry;

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

    virtual bool visit_(Value const& v, Pointer const& p)
    {
        m_value = wrap_value(v, m_registry, cValue);
        return false;
    }
    virtual bool visit_(Value const& v, Array const& a) 
    {
        m_value = wrap_value(v, m_registry, cValueArray);
        return false;
    }
    virtual bool visit_(Value const& v, Compound const& c)
    { 
        m_value = wrap_value(v, m_registry, cValue);
        return false; 
    }
    virtual bool visit_(Enum::integral_type& v, Enum const& e)   
    { 
        m_value = enum_get_rb_symbol(v, e);
        return false;
    }
    
public:
    RubyGetter()
        : ValueVisitor(false) {}

    VALUE apply(Typelib::Value value, VALUE registry)
    {
        m_registry = registry;
        m_value = Qnil;
        try { 
            ValueVisitor::apply(value);
            return m_value;
        }
        catch(UnsupportedType) { return Qnil; }
    }
};

/* Only PODs are assignable, moreover pointers are followed */
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

    virtual bool visit_(Value const& v, Array const&)      { throw UnsupportedType(v.getType()); }
    virtual bool visit_(Value const& v, Compound const&)   { throw UnsupportedType(v.getType()); }
    virtual bool visit_(Enum::integral_type& v, Enum const& e)
    { 
        v = rb_enum_get_value(m_value, e);
        return false;
    }
    
public:
    RubySetter()
        : ValueVisitor(false) {}

    bool apply(Value value, VALUE new_value)
    {
        m_value = new_value;
        try { 
            ValueVisitor::apply(value); 
            return new_value;
        }
        catch(UnsupportedType)      { return Qnil; }
    }
};

/*
 * Convertion function between Ruby and Typelib
 */


/* Converts a Typelib::Value to Ruby's VALUE */
static VALUE typelib_to_ruby(Value v, VALUE registry)
{ 
    RubyGetter getter;
    return getter.apply(v, registry);
}

/* Converts a given field in +value+ */
static VALUE typelib_to_ruby(Value value, VALUE name, VALUE registry)
{
    try { 
        Value field_value = value_get_field(value, StringValuePtr(name));
        return typelib_to_ruby(field_value, registry);
    } 
    catch(FieldNotFound)   { return Qnil; } 
}

static VALUE typelib_to_ruby(VALUE value, VALUE name)
{ 
    // Get the registry
    VALUE registry = typelib_value_get_registry(value);
    return typelib_to_ruby(rb_get_cxx<Value>(value), name, registry); 
}

/* Converts a Ruby's VALUE to Typelib::Value */
static VALUE typelib_from_ruby(Value value, VALUE new_value)
{
    RubySetter setter;
    return setter.apply(value, new_value);
}

/* Sets a given field in +value+ */
static VALUE typelib_from_ruby(VALUE value, VALUE name, VALUE new_value)
{
    Value& tlib_value(rb_get_cxx<Value>(value));
    try { 
        Value field_value = value_get_field(tlib_value, StringValuePtr(name));
        typelib_from_ruby(field_value, new_value);
        return new_value;
    } 
    catch(FieldNotFound)   { return Qnil; } 
}


