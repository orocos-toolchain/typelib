static Array const& Typelib_Get_Array(Value& value)
{
    try { return dynamic_cast<Array const&>(value.getType()); }
    catch(...)
    { rb_raise(rb_eTypeError, "expecting a ValueArray"); }
}

static Value typelib_array_element(VALUE rbarray, VALUE rbindex)
{
    Value& value(rb_get_cxx<Value>(rbarray));
    Array const& array(Typelib_Get_Array(value));
    size_t index = NUM2INT(rbindex);
    
    if (array.getDimension() < index)
    {
        rb_raise(rb_eIndexError, "Out of bounds: %i > %i", index, array.getDimension());
        return Value();
    }

    Type const& array_type(array.getIndirection());

    int8_t* data = reinterpret_cast<int8_t*>(value.getData());
    data += array_type.getSize() * index;
    return Value(data, array_type);
}

static VALUE typelib_array_get(VALUE self, VALUE rbindex)
{ 
    Value element = typelib_array_element(self, rbindex);
    VALUE registry = typelib_value_get_registry(self);

    return typelib_to_ruby(element, registry); 
}

static VALUE typelib_array_set(VALUE self, VALUE rbindex, VALUE newvalue)
{ 
    Value element = typelib_array_element(self, rbindex);
    return typelib_from_ruby(element, newvalue); 
}

static VALUE typelib_array_each(VALUE rbarray)
{
    Value& value(rb_get_cxx<Value>(rbarray));
    Array const& array(Typelib_Get_Array(value));
    Type  const& array_type(array.getIndirection());
    VALUE registry(typelib_value_get_registry(rbarray));

    int8_t* data = reinterpret_cast<int8_t*>(value.getData());

    VALUE last_value = Qnil;
    for (size_t i = 0; i < array.getDimension(); ++i, data += array_type.getSize())
        last_value = rb_yield( typelib_to_ruby( Value(data, array_type), registry ) );

    return last_value;
}

static VALUE typelib_array_size(VALUE rbarray)
{
    Value& value(rb_get_cxx<Value>(rbarray));
    Array const& array(Typelib_Get_Array(value));
    return INT2FIX(array.getDimension());
}


