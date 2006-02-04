namespace cxx2rb
{
    static Value array_element(VALUE rbarray, VALUE rbindex)
    {
        Value& value(rb2cxx::object<Value>(rbarray));
        Array const& array(static_cast<Array const&>(value.getType()));
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
}

static VALUE array_get(VALUE self, VALUE rbindex)
{ 
    Value element = cxx2rb::array_element(self, rbindex);
    VALUE registry = value_get_registry(self);
    return typelib_to_ruby(element, registry); 
}

static VALUE array_set(VALUE self, VALUE rbindex, VALUE newvalue)
{ 
    Value element = cxx2rb::array_element(self, rbindex);
    return typelib_from_ruby(element, newvalue); 
}

static VALUE array_each(VALUE rbarray)
{
    Value& value            = rb2cxx::object<Value>(rbarray);
    Array const& array      = static_cast<Array const&>(value.getType());
    Type  const& array_type = array.getIndirection();
    VALUE registry          = value_get_registry(rbarray);

    int8_t* data = reinterpret_cast<int8_t*>(value.getData());
    VALUE last_value = Qnil;
    for (size_t i = 0; i < array.getDimension(); ++i, data += array_type.getSize())
        last_value = rb_yield( typelib_to_ruby( Value(data, array_type), registry ) );

    return last_value;
}

static VALUE array_size(VALUE rbarray)
{
    Value& value(rb2cxx::object<Value>(rbarray));
    Array const& array(static_cast<Array const&>(value.getType()));
    return INT2FIX(array.getDimension());
}


