#include "typelib.hh"
extern "C" {
#include <dl.h>
}

using namespace Typelib;

/**********************************************
 * Typelib::Compound
 */

static VALUE compound_get_fields(VALUE self)
{
    Type const& type(rb2cxx::object<Type>(self));
    Compound const& compound(dynamic_cast<Compound const&>(type));
    Compound::FieldList const& fields(compound.getFields());
    Compound::FieldList::const_iterator it, end = fields.end();

    VALUE registry = rb_iv_get(self, "@registry");
    VALUE fieldlist = rb_ary_new();
    for (it = fields.begin(); it != end; ++it)
    {
        VALUE field_name = rb_str_new2(it->getName().c_str());
        VALUE field_type = cxx2rb::type_wrap(it->getType(), registry);

        VALUE field_def = rb_ary_new2(3);
        rb_ary_store(field_def, 0, field_name);
        rb_ary_store(field_def, 1, INT2FIX(it->getOffset()));
        rb_ary_store(field_def, 2, field_type);
        rb_ary_push(fieldlist, field_def);
    }

    return fieldlist;
}

/* :nodoc:
 * Helper function for CompoundType#[] */
static VALUE compound_field_get(VALUE rbvalue, VALUE name)
{ 
    VALUE registry = value_get_registry(rbvalue);
    Value value = rb2cxx::object<Value>(rbvalue);
    if (! value.getData())
        return Qnil;

    try { 
        Value field_value = value_get_field(value, StringValuePtr(name));
	if (value.getData() == field_value.getData())
	    return typelib_to_ruby(field_value, registry, Qnil, rb_iv_get(rbvalue, "@ptr"));
	else
	    return typelib_to_ruby(field_value, registry, rbvalue, Qnil);
    } 
    catch(FieldNotFound)   {} 
    rb_raise(rb_eArgError, "no field '%s'", StringValuePtr(name));
}
/* :nodoc:
 * Helper function for CompoundType#[]= */
static VALUE compound_field_set(VALUE self, VALUE name, VALUE newval)
{ 
    Value& tlib_value(rb2cxx::object<Value>(self));

    try {
        Value field_value = value_get_field(tlib_value, StringValuePtr(name));
        typelib_from_ruby(field_value, newval);
        return newval;
    } catch(FieldNotFound) {}
    rb_raise(rb_eArgError, "no field '%s' in '%s'", StringValuePtr(name), rb_obj_classname(self));
}

/* Returns the value pointed to by +self+ */
static VALUE pointer_deference(VALUE self)
{
    VALUE pointed_to = rb_iv_get(self, "@pointed_object");
    if (!NIL_P(pointed_to))
	return pointed_to;

    Value const& value(rb2cxx::object<Value>(self));
    Indirect const& indirect(static_cast<Indirect const&>(value.getType()));
    
    VALUE registry = value_get_registry(self);

    Value new_value( *reinterpret_cast<void**>(value.getData()), indirect.getIndirection() );
    VALUE dlptr = rb_dlptr_new(new_value.getData(), new_value.getType().getSize(), 0);
    return typelib_to_ruby(new_value, registry, Qnil, dlptr);
}


namespace rb2cxx {
    /* Get the C value associated with a ruby representation of an enum
     * Valid ruby representations are: symbols, strings and integer
     */
    Enum::integral_type enum_value(VALUE rb_value, Enum const& e)
    {
        // m_value can be either an integer, a symbol or a string
        if (TYPE(rb_value) == T_FIXNUM)
        {
            Enum::integral_type value = FIX2INT(rb_value);
            try { 
		e.get(value); 
		return value;
	    }
            catch(Enum::ValueNotFound) {  }
	    rb_raise(rb_eArgError, "%i is not a valid value for %s", value, e.getName().c_str());
        }

        char const* name;
        if (SYMBOL_P(rb_value))
            name = rb_id2name(SYM2ID(rb_value));
        else
            name = StringValuePtr(rb_value);

        try { return e.get(name); }
        catch(Enum::SymbolNotFound) {  }
	rb_raise(rb_eArgError, "%s is not a valid symbol for %s", name, e.getName().c_str());
    }
}

VALUE enum_keys(VALUE self)
{
    Enum const& type = static_cast<Enum const&>(rb2cxx::object<Type>(self));

    VALUE keys = rb_iv_get(self, "@values");
    if (!NIL_P(keys)) 
        return keys;

    keys = rb_hash_new();
    typedef std::list<std::string> string_list;
    string_list names = type.names();
    for (string_list::const_iterator it = names.begin(); it != names.end(); ++it)
        rb_hash_aset(keys, rb_str_new2(it->c_str()), INT2FIX(type.get(*it)));

    rb_iv_set(self, "@values", keys);
    return keys;
}

/**********************************************
 * Typelib::Pointer
 */

static VALUE indirect_type_deference(VALUE self)
{
    VALUE registry = rb_iv_get(self, "@registry");
    Type const& type(rb2cxx::object<Type>(self));
    Indirect const& indirect(static_cast<Indirect const&>(type));
    return cxx2rb::type_wrap(indirect.getIndirection(), registry);
}


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

/* call-seq:
 *  array[index]    => value
 *
 * Returns the value at +index+ in the array. Since Typelib arrays are *not*
 * dynamically extensible, trying to set a non-existent index will raise an
 * IndexError exception.
 */
static VALUE array_get(VALUE self, VALUE rbindex)
{ 
    Value element = cxx2rb::array_element(self, rbindex);
    VALUE registry = value_get_registry(self);

    if (FIX2INT(rbindex))
	return typelib_to_ruby(element, registry, self, Qnil); 
    else
	return typelib_to_ruby(element, registry, Qnil, rb_iv_get(self, "@ptr")); 
}

/* call-seq:
 *  array[index] = new_value    => new_value
 *
 * Sets the value at +index+ to +new_value+. Since Typelib arrays are *not*
 * dynamically extensible, trying to set a non-existent index will raise an
 * IndexError exception.
 */
static VALUE array_set(VALUE self, VALUE rbindex, VALUE newvalue)
{ 
    Value element = cxx2rb::array_element(self, rbindex);
    return typelib_from_ruby(element, newvalue); 
}

/* call-seq:
 *  array.each { |v| ... }	=> array
 *
 * Iterates on all elements of the array
 */
static VALUE array_each(VALUE rbarray)
{
    Value& value            = rb2cxx::object<Value>(rbarray);
    Array const& array      = static_cast<Array const&>(value.getType());
    if (array.getDimension() == 0)
	return rbarray;

    Type  const& array_type = array.getIndirection();
    VALUE registry          = value_get_registry(rbarray);

    int8_t* data = reinterpret_cast<int8_t*>(value.getData());

    rb_yield(typelib_to_ruby( Value(data, array_type), registry, Qnil, rb_iv_get(rbarray, "@ptr") ));
    data += array_type.getSize();
    for (size_t i = 1; i < array.getDimension(); ++i, data += array_type.getSize())
	rb_yield(typelib_to_ruby( Value(data, array_type), registry, rbarray, Qnil ));

    return rbarray;
}

/* call-seq:
 *  array.size		    => size
 *
 * Returns the array size
 */
static VALUE array_size(VALUE rbarray)
{
    Value& value(rb2cxx::object<Value>(rbarray));
    Array const& array(static_cast<Array const&>(value.getType()));
    return INT2FIX(array.getDimension());
}

/*
 * call-seq:
 *  pointer.null?		=> boolean
 *
 * checks if this is a NULL pointer 
 */
static VALUE pointer_nil_p(VALUE self)
{
    Value const& value(rb2cxx::object<Value>(self));
    if ( *reinterpret_cast<void**>(value.getData()) == 0 )
        return Qtrue;
    return Qfalse;
}


void Typelib_init_specialized_types()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    cIndirect  = rb_define_class_under(mTypelib, "IndirectType", cType);
    rb_define_singleton_method(cIndirect, "deference",    RUBY_METHOD_FUNC(indirect_type_deference), 0);

    cPointer  = rb_define_class_under(mTypelib, "PointerType", cIndirect);
    rb_define_method(cPointer, "deference", RUBY_METHOD_FUNC(pointer_deference), 0);
    rb_define_method(cPointer, "null?", RUBY_METHOD_FUNC(pointer_nil_p), 0);
    
    cCompound = rb_define_class_under(mTypelib, "CompoundType", cType);
    rb_define_singleton_method(cCompound, "get_fields",   RUBY_METHOD_FUNC(compound_get_fields), 0);
// rb_define_singleton_method(cCompound, "writable?",  RUBY_METHOD_FUNC(compound_field_is_writable), 1);
    rb_define_method(cCompound, "get_field", RUBY_METHOD_FUNC(compound_field_get), 1);
    rb_define_method(cCompound, "set_field", RUBY_METHOD_FUNC(compound_field_set), 2);

    cEnum = rb_define_class_under(mTypelib, "EnumType", cType);
    rb_define_singleton_method(cEnum, "keys", RUBY_METHOD_FUNC(enum_keys), 0);

    cArray    = rb_define_class_under(mTypelib, "ArrayType", cIndirect);
    rb_define_method(cArray, "[]",      RUBY_METHOD_FUNC(array_get), 1);
    rb_define_method(cArray, "[]=",     RUBY_METHOD_FUNC(array_set), 2);
    rb_define_method(cArray, "each",    RUBY_METHOD_FUNC(array_each), 0);
    rb_define_method(cArray, "size",    RUBY_METHOD_FUNC(array_size), 0);
}

