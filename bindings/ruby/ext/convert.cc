#include "typelib.hh"
#include <iostream>
#include <typelib/value_ops.hh>

using namespace Typelib;
using namespace typelib_ruby;

/**********
 *  Define typelib_(to|from)_ruby
 */

/* This visitor takes a Value class and a field name,
 * and returns the VALUE object which corresponds to
 * the field, or returns nil
 */
bool RubyGetter::visit_(int8_t &value) {
    m_value = INT2FIX(value);
    return false;
}
bool RubyGetter::visit_(uint8_t &value) {
    m_value = INT2FIX(value);
    return false;
}
bool RubyGetter::visit_(int16_t &value) {
    m_value = INT2FIX(value);
    return false;
}
bool RubyGetter::visit_(uint16_t &value) {
    m_value = INT2FIX(value);
    return false;
}
bool RubyGetter::visit_(int32_t &value) {
    m_value = INT2NUM(value);
    return false;
}
bool RubyGetter::visit_(uint32_t &value) {
    m_value = INT2NUM(value);
    return false;
}
bool RubyGetter::visit_(int64_t &value) {
    m_value = LL2NUM(value);
    return false;
}
bool RubyGetter::visit_(uint64_t &value) {
    m_value = ULL2NUM(value);
    return false;
}
bool RubyGetter::visit_(float &value) {
    m_value = rb_float_new(value);
    return false;
}
bool RubyGetter::visit_(double &value) {
    m_value = rb_float_new(value);
    return false;
}

bool RubyGetter::visit_(Value const &v, Pointer const &p) {
#ifdef VERBOSE
    fprintf(stderr, "%p: wrapping from RubyGetter::visit_(pointer)\n",
            v.getData());
#endif
    m_value = cxx2rb::value_wrap(v, m_registry, m_parent);
    return false;
}
bool RubyGetter::visit_(Value const &v, Array const &a) {
#ifdef VERBOSE
    fprintf(stderr, "%p: wrapping from RubyGetter::visit_(array)\n",
            v.getData());
#endif
    m_value = cxx2rb::value_wrap(v, m_registry, m_parent);
    return false;
}
bool RubyGetter::visit_(Value const &v, Compound const &c) {
#ifdef VERBOSE
    fprintf(stderr, "%p: wrapping from RubyGetter::visit_(compound)\n",
            v.getData());
#endif
    m_value = cxx2rb::value_wrap(v, m_registry, m_parent);
    return false;
}
bool RubyGetter::visit_(Value const &v, OpaqueType const &c) {
#ifdef VERBOSE
    fprintf(stderr, "%p: wrapping from RubyGetter::visit_(opaque)\n",
            v.getData());
#endif
    m_value = cxx2rb::value_wrap(v, m_registry, m_parent);
    return false;
}
bool RubyGetter::visit_(Value const &v, Container const &c) {
#ifdef VERBOSE
    fprintf(stderr, "%p: wrapping from RubyGetter::visit_(container)\n",
            v.getData());
#endif
    m_value = cxx2rb::value_wrap(v, m_registry, m_parent);
    return false;
}
bool RubyGetter::visit_(Enum::integral_type &v, Enum const &e) {
    m_value = cxx2rb::enum_symbol(v, e);
    return false;
}

RubyGetter::RubyGetter() : ValueVisitor(false) {}
RubyGetter::~RubyGetter() {
    m_value = Qnil;
    m_registry = Qnil;
}

VALUE RubyGetter::apply(Typelib::Value value, VALUE registry, VALUE parent) {
    m_registry = registry;
    m_value = Qnil;
    m_parent = parent;

    ValueVisitor::apply(value);
    return m_value;
}

bool RubySetter::visit_(int8_t &value) {
    value = NUM2INT(m_value);
    return false;
}
bool RubySetter::visit_(uint8_t &value) {
    value = NUM2INT(m_value);
    return false;
}
bool RubySetter::visit_(int16_t &value) {
    value = NUM2INT(m_value);
    return false;
}
bool RubySetter::visit_(uint16_t &value) {
    value = NUM2UINT(m_value);
    return false;
}
bool RubySetter::visit_(int32_t &value) {
    value = NUM2INT(m_value);
    return false;
}
bool RubySetter::visit_(uint32_t &value) {
    value = NUM2UINT(m_value);
    return false;
}
bool RubySetter::visit_(int64_t &value) {
    value = NUM2LL(m_value);
    return false;
}
bool RubySetter::visit_(uint64_t &value) {
    value = NUM2LL(m_value);
    return false;
}
bool RubySetter::visit_(float &value) {
    value = NUM2DBL(m_value);
    return false;
}
bool RubySetter::visit_(double &value) {
    value = NUM2DBL(m_value);
    return false;
}

bool RubySetter::visit_(Value const &v, Array const &a) {
    if (a.getIndirection().getName() == "/char") {
        char *value = StringValuePtr(m_value);
        size_t length = strlen(value);
        if (length < a.getDimension()) {
            memcpy(v.getData(), value, length + 1);
            return false;
        }
        throw UnsupportedType(v.getType(), "string too long");
    }
    throw UnsupportedType(v.getType(), "not a string");
}
bool RubySetter::visit_(Value const &v, Pointer const &c) {
    throw UnsupportedType(v.getType(), "no conversion to pointers");
}
bool RubySetter::visit_(Value const &v, Compound const &c) {
    throw UnsupportedType(v.getType(), "no conversion to compound");
}
bool RubySetter::visit_(Value const &v, OpaqueType const &c) {
    throw UnsupportedType(v.getType(), "no conversion to opaque types");
}
bool RubySetter::visit_(Value const &v, Container const &c) {
    throw UnsupportedType(v.getType(), "no conversion to containers");
}
bool RubySetter::visit_(Enum::integral_type &v, Enum const &e) {
    v = rb2cxx::enum_value(m_value, e);
    return false;
}

RubySetter::RubySetter() : ValueVisitor(false) {}
RubySetter::~RubySetter() { m_value = Qnil; }

VALUE RubySetter::apply(Value value, VALUE new_value) {
    m_value = new_value;
    ValueVisitor::apply(value);
    return new_value;
}

/*
 * Convertion function between Ruby and Typelib
 */

/* Converts a Typelib::Value to Ruby's VALUE */
VALUE typelib_to_ruby(Value v, VALUE registry, VALUE parent) {
    if (!v.getData())
        return Qnil;

    RubyGetter getter;
    return getter.apply(v, registry, parent);
}

/* Returns the Value object wrapped into +value+ */
Value typelib_get(VALUE value) {
    void *object = 0;
    Data_Get_Struct(value, void, object);
    return *reinterpret_cast<Value *>(object);
}

/* Tries to initialize +value+ to +new_value+ using the type in +value+ */
VALUE typelib_from_ruby(Value dst, VALUE new_value) {
    // Special case: new_value is actually a Typelib wrapper of the right type
    if (rb_obj_is_kind_of(new_value, cType)) {
        Value &src = rb2cxx::object<Value>(new_value);
        Type const &dst_t = dst.getType();
        Type const &src_t = src.getType();
        if (dst_t == src_t)
            Typelib::copy(dst, src);
        else {
            rb_raise(rb_eArgError, "wrong type in assignment: %s = %s",
                     dst_t.getName().c_str(), src_t.getName().c_str());
        }
        return new_value;
    }

    std::string type_name;
    std::string reason;
    try {
        RubySetter setter;
        return setter.apply(dst, new_value);
    } catch (UnsupportedType e) {
        // Avoid calling rb_raise in exception context
        type_name = e.type.getName();
        reason = e.reason;
    }

    if (reason.length() == 0)
        rb_raise(rb_eTypeError, "cannot convert to '%s'", type_name.c_str());
    else
        rb_raise(rb_eTypeError, "cannot convert to '%s' (%s)",
                 type_name.c_str(), reason.c_str());
}
