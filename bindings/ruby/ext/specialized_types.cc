#include "typelib.hh"
#include <iostream>

#include <boost/bind.hpp>
#include <typelib/value_ops.hh>

using namespace Typelib;
using std::vector;
using namespace typelib_ruby;

/**********************************************
 * Typelib::Compound
 */

static VALUE compound_get_fields(VALUE self)
{
    Type const& type(rb2cxx::object<Type>(self));
    Compound const& compound(dynamic_cast<Compound const&>(type));
    Compound::FieldList const& fields(compound.getFields());
    Compound::FieldList::const_iterator it, end = fields.end();

    VALUE registry  = type_get_registry(self);
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

/* Helper function for CompoundType#[] */
static VALUE compound_field_get(VALUE rbvalue, VALUE name)
{ 
    VALUE registry = value_get_registry(rbvalue);
    Value value = rb2cxx::object<Value>(rbvalue);
    if (! value.getData())
        return Qnil;

    try { 
        Value field_value = value_get_field(value, StringValuePtr(name));
	return typelib_to_ruby(field_value, registry, rbvalue);
    } 
    catch(FieldNotFound)
    { rb_raise(rb_eArgError, "no field '%s'", StringValuePtr(name)); } 
    catch(std::runtime_error e)
    { rb_raise(rb_eRuntimeError, e.what()); }
}
/* Helper function for CompoundType#[]= */
static VALUE compound_field_set(VALUE self, VALUE name, VALUE newval)
{ 
    Value& tlib_value(rb2cxx::object<Value>(self));

    try {
        Value field_value = value_get_field(tlib_value, StringValuePtr(name));
        typelib_from_ruby(field_value, newval);
        return newval;
    }
    catch(FieldNotFound)
    { rb_raise(rb_eArgError, "no field '%s' in '%s'", StringValuePtr(name), rb_obj_classname(self)); }
    catch(std::runtime_error e)
    { rb_raise(rb_eRuntimeError, e.what()); }
}

/* call-seq:
 *  pointer.deference => pointed-to type
 *
 * Returns the value pointed to by +self+ 
 */
static VALUE pointer_deference(VALUE self)
{
    VALUE pointed_to = rb_iv_get(self, "@points_to");
    if (!NIL_P(pointed_to))
	return pointed_to;

    Value const& value(rb2cxx::object<Value>(self));
    Indirect const& indirect(static_cast<Indirect const&>(value.getType()));
    
    VALUE registry = value_get_registry(self);

    void* ptr_value = *reinterpret_cast<void**>(value.getData());
    if (!ptr_value)
        rb_raise(rb_eArgError, "cannot deference a NULL pointer");

    Value new_value(ptr_value, indirect.getIndirection() );
    VALUE ptr = memory_wrap(new_value.getData());
    return typelib_to_ruby(new_value, registry, Qnil);
}


/* Get the C value associated with a ruby representation of an enum
 * Valid ruby representations are: symbols, strings and integer
 */
Enum::integral_type rb2cxx::enum_value(VALUE rb_value, Enum const& e)
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

/* call-seq:
 *  enum.keys => array of keys
 *
 * Returns a string array of all the keys defined for this enumeration
 */
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

/* call-seq:
 *  enum.value_of(name) => integer
 *
 * Returns the integral value asoociated with the given name.
 */
static VALUE enum_value_of(VALUE self, VALUE name)
{
    Enum const& type = static_cast<Enum const&>(rb2cxx::object<Type>(self));

    try {
        int value = type.get(StringValuePtr(name));
        return INT2NUM(value);
    } catch (Enum::SymbolNotFound) {
        rb_raise(rb_eArgError, "this enumeration has no value for %s", StringValuePtr(name));
    }
}

/* call-seq:
 *  enum.name_of(integer) => key
 *
 * Returns the symbolic name of +integer+ in this enumeration, or raises
 * ArgumentError if there is no matching name.
 */
static VALUE enum_name_of(VALUE self, VALUE integer)
{
    Enum const& type = static_cast<Enum const&>(rb2cxx::object<Type>(self));

    try {
        std::string name = type.get(NUM2INT(integer));
        return rb_str_new2(name.c_str());
    } catch (Enum::ValueNotFound) {
        rb_raise(rb_eArgError, "this enumeration has no name for %i", NUM2INT(integer));
    }
}

/**********************************************
 * Typelib::Pointer
 */

/* call-seq:
 *  type.deference => other_type
 *
 * Returns the type referenced by this one
 */
static VALUE indirect_type_deference(VALUE self)
{
    VALUE registry = type_get_registry(self);
    Type const& type(rb2cxx::object<Type>(self));
    Indirect const& indirect(static_cast<Indirect const&>(type));
    return cxx2rb::type_wrap(indirect.getIndirection(), registry);
}


static Value array_element(VALUE rbarray, VALUE rbindex)
{
    Value& value(rb2cxx::object<Value>(rbarray));
    Array const& array(static_cast<Array const&>(value.getType()));
    size_t index = NUM2INT(rbindex);
    
    if (index >= array.getDimension())
    {
        rb_raise(rb_eIndexError, "Out of bounds: %i > %i", index, array.getDimension());
        return Value();
    }

    Type const& array_type(array.getIndirection());

    int8_t* data = reinterpret_cast<int8_t*>(value.getData());
    data += array_type.getSize() * index;
    return Value(data, array_type);
}

/* call-seq:
 *  array[index]    => value
 *
 * Returns the value at +index+ in the array. Since Typelib arrays are *not*
 * dynamically extensible, trying to set a non-existent index will raise an
 * IndexError exception.
 */
static VALUE array_get(int argc, VALUE* argv, VALUE self)
{ 
    Value& value            = rb2cxx::object<Value>(self);
    Array const& array      = static_cast<Array const&>(value.getType());
    if (array.getDimension() == 0)
	return self;

    Type  const& array_type = array.getIndirection();
    VALUE registry          = value_get_registry(self);

    int8_t* data = reinterpret_cast<int8_t*>(value.getData());
    size_t index = NUM2INT(argv[0]);
    if (index >= array.getDimension())
	rb_raise(rb_eIndexError, "Out of bounds: %i > %i", index, array.getDimension());

    if (argc == 1)
    {
	Value v = Value(data + array_type.getSize() * index, array_type);
	return typelib_to_ruby( v, registry, self );
    }
    else if (argc == 2)
    {
	VALUE ret = rb_ary_new();
	size_t size = NUM2INT(argv[1]);
	if (index + size > array.getDimension())
	    rb_raise(rb_eIndexError, "Out of bounds: %i > %i", index + size - 1, array.getDimension());

	for (size_t i = index; i < index + size; ++i)
	{
	    Value v = Value(data + array_type.getSize() * i, array_type);
	    VALUE rb_v = typelib_to_ruby( v, registry, self );

	    rb_ary_push(ret, rb_v);
	}

	return ret;
    }
    else
	rb_raise(rb_eArgError, "invalid argument count (%i for 1 or 2)", argc);
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
    Value element = array_element(self, rbindex);
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

    for (size_t i = 0; i < array.getDimension(); ++i, data += array_type.getSize())
	rb_yield(typelib_to_ruby( Value(data, array_type), registry, rbarray ));

    return rbarray;
}

/* call-seq:
 *  array.size		    => size
 *
 * Returns the count of elements in +array+
 */
static VALUE array_size(VALUE rbarray)
{
    Value& value(rb2cxx::object<Value>(rbarray));
    Array const& array(static_cast<Array const&>(value.getType()));
    return INT2FIX(array.getDimension());
}

/* call-seq:
 *  array.length		    => length
 *
 * Returns the count of elemnts in +array+
 */
static VALUE array_class_length(VALUE rbarray)
{
    Array const& array(dynamic_cast<Array const&>(rb2cxx::object<Type>(rbarray)));
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


/*
 * call-seq:
 *  numeric.integer?	    => true or false
 *
 * Returns true if the type is an integral type and false if it is a floating-point type
 */
static VALUE numeric_type_integer_p(VALUE self)
{
    Numeric const& type(dynamic_cast<Numeric const&>(rb2cxx::object<Type>(self)));
    return type.getNumericCategory() == Numeric::Float ? Qfalse : Qtrue;
}

/*
 * call-seq:
 *  numeric.size	    => value
 *
 * The size of this type in bytes
 */
static VALUE numeric_type_size(VALUE self)
{
    Numeric const& type(dynamic_cast<Numeric const&>(rb2cxx::object<Type>(self)));
    return INT2FIX(type.getSize());
}

/*
 * call-seq:
 *  numeric.unsigned?	    => value
 *
 * If integer? returns true, returns whether this type is an unsigned or signed
 * integral type.
 */
static VALUE numeric_type_unsigned_p(VALUE self)
{
    Numeric const& type(dynamic_cast<Numeric const&>(rb2cxx::object<Type>(self)));
    switch(type.getNumericCategory())
    {
	case Numeric::SInt: return Qfalse;
	case Numeric::UInt: return Qtrue;
	case Numeric::Float:
	    rb_raise(rb_eArgError, "not an integral type");
    }
    return Qnil; // never reached
}


/*
 * call-seq:
 *  klass.container_name => name
 *
 * Returns the name of the generic container. For instance, a
 * /std/vector</double> type would return /std/vector.
 */
static VALUE container_kind(VALUE self)
{
    Container const& type(dynamic_cast<Container const&>(rb2cxx::object<Type>(self)));
    return rb_str_new2(type.kind().c_str());
}

/*
 * call-seq:
 *  container.length => value
 *
 * Returns the count of elements in +container+
 */
static VALUE container_length(VALUE self)
{
    Value value = rb2cxx::object<Value>(self);
    Container const& type(dynamic_cast<Container const&>(value.getType()));

    return INT2NUM(type.getElementCount(value.getData()));
}

/*
 * call-seq:
 *  container.insert(element) => container
 *
 * Inserts a new element in the container
 */
static VALUE container_insert(VALUE self, VALUE obj)
{
    Value container_v = rb2cxx::object<Value>(self);
    Container const& container_t(dynamic_cast<Container const&>(container_v.getType()));

    Type const& element_t = container_t.getIndirection();
    vector<int8_t> buffer(element_t.getSize());
    Value v(&buffer[0], element_t);
    Typelib::init(v);
    typelib_from_ruby(v, obj);

    container_t.insert(container_v.getData(), v);
    return self;
}

struct ContainerIterator : public RubyGetter
{
    bool inside;

    bool visit_(Value const& v, Container const& c)
    { 
        if (inside)
            RubyGetter::visit_(v, c);
        else
            ValueVisitor::visit_(v, c);
        return false;
    }

    void apply(Value v, VALUE registry, VALUE parent)
    {
        inside = false;
        RubyGetter::apply(v, registry, parent);
    }
    virtual void dispatch(Value v)
    {
        inside = true;
        RubyGetter::dispatch(v);

        VALUE result = m_value;
        m_value = Qnil;
        rb_yield(result);
    }
};

/*
 * call-seq:
 *  container.each { |obj| ... } => container
 *
 * Iterates on the elements of the container
 */
static VALUE container_each(VALUE self)
{
    Value value = rb2cxx::object<Value>(self);
    Container const& type(dynamic_cast<Container const&>(value.getType()));

    VALUE registry = value_get_registry(self);
    ContainerIterator iterator;
    iterator.apply(value, registry, self);
    return self;
}

/*
 * call-seq:
 *  container.erase(obj) => true or false
 *
 * Removes +obj+ from the container. Returns true if +obj+ has been found and
 * deleted, and false otherwise
 */
static VALUE container_erase(VALUE self, VALUE obj)
{
    Value container_v = rb2cxx::object<Value>(self);
    Container const& container_t(dynamic_cast<Container const&>(container_v.getType()));

    Type const& element_t = container_t.getIndirection();
    vector<int8_t> buffer(element_t.getSize());
    Value v(&buffer[0], element_t);
    Typelib::init(v);
    typelib_from_ruby(v, obj);

    if (container_t.erase(container_v.getData(), v))
        return Qtrue;
    else
        return Qfalse;
}

bool container_delete_if_i(Value v, VALUE registry, VALUE container)
{
    VALUE rb_v = typelib_to_ruby(v, registry, container);
    if (RTEST(rb_yield(rb_v)))
        return true;
    return false;
}

/*
 * call-seq:
 *  container.delete_if { |obj| ... } => container
 *
 * Removes the elements in the container for which the block returns true.
 */
static VALUE container_delete_if(VALUE self)
{
    Value container_v = rb2cxx::object<Value>(self);
    Container const& container_t(dynamic_cast<Container const&>(container_v.getType()));

    VALUE registry = value_get_registry(self);
    container_t.delete_if(container_v.getData(), boost::bind(container_delete_if_i, _1, registry, self));
    return self;
}

/* Document-class: Typelib::NumericType
 *
 * Wrapper for numeric types (int, float, ...)
 */

/* Document-class: Typelib::IndirectType
 *
 * Base class for types which deference other objects, like pointers or arrays
 * See also ArrayType and PointerType
 */

/* Document-class: Typelib::PointerType
 *
 * Base class for pointers (references to other values)
 */

/* Document-class: Typelib::CompoundType
 *
 * Base class for types composed by other types (structures and unions in C).
 * Each field is assigned its type and its offset inside the structure. So
 * unions are represented with all fields having an offset of zero, while
 * structures have consecutive offsets.
 */

void typelib_ruby::Typelib_init_specialized_types()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    cNumeric   = rb_define_class_under(mTypelib, "NumericType", cType);
    rb_define_singleton_method(cNumeric, "integer?", RUBY_METHOD_FUNC(numeric_type_integer_p), 0);
    rb_define_singleton_method(cNumeric, "unsigned?", RUBY_METHOD_FUNC(numeric_type_unsigned_p), 0);
    rb_define_singleton_method(cNumeric, "size", RUBY_METHOD_FUNC(numeric_type_size), 0);

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
    rb_define_singleton_method(cEnum, "value_of",      RUBY_METHOD_FUNC(enum_value_of), 1);
    rb_define_singleton_method(cEnum, "name_of",      RUBY_METHOD_FUNC(enum_name_of), 1);

    cArray    = rb_define_class_under(mTypelib, "ArrayType", cIndirect);
    rb_define_singleton_method(cArray, "length", RUBY_METHOD_FUNC(array_class_length), 0);
    rb_define_method(cArray, "[]",      RUBY_METHOD_FUNC(array_get), -1);
    rb_define_method(cArray, "[]=",     RUBY_METHOD_FUNC(array_set), 2);
    rb_define_method(cArray, "each",    RUBY_METHOD_FUNC(array_each), 0);
    rb_define_method(cArray, "size",    RUBY_METHOD_FUNC(array_size), 0);

    cContainer = rb_define_class_under(mTypelib, "ContainerType", cIndirect);
    rb_define_singleton_method(cContainer, "container_kind", RUBY_METHOD_FUNC(container_kind), 0);
    rb_define_method(cContainer, "length",    RUBY_METHOD_FUNC(container_length), 0);
    rb_define_method(cContainer, "insert",    RUBY_METHOD_FUNC(container_insert), 1);
    rb_define_method(cContainer, "each",      RUBY_METHOD_FUNC(container_each), 0);
    rb_define_method(cContainer, "erase",     RUBY_METHOD_FUNC(container_erase), 1);
    rb_define_method(cContainer, "delete_if", RUBY_METHOD_FUNC(container_delete_if), 0);
}

