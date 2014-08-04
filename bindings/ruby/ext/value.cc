#include "typelib.hh"
#include <ruby.h>

#include <sstream>
#include <limits>
#include <typelib/value.hh>
#include <typelib/registry.hh>
#include <typelib/typevisitor.hh>
#include <typelib/csvoutput.hh>
#include <typelib/endianness.hh>
#include <typelib/value_ops.hh>
#include <typelib/typename.hh>

#include <iostream>

using namespace Typelib;
using std::numeric_limits;
using std::vector;
using namespace typelib_ruby;

void *value_root_ptr(VALUE value) {
    VALUE parent = Qnil;
    while (RTEST(value)) {
        parent = value;
        value = rb_iv_get(value, "@parent");
    }
    if (RTEST(parent)) {
        Value v = rb2cxx::object<Value>(parent);
        return v.getData();
    } else
        return 0;
}

/* There are constraints when creating a Ruby wrapper for a Type, mainly
 * for avoiding GC issues. This function does the work.
 *
 * The main issue is that Ruby/DL does not keep refcounters on memory.
 * Instead, it returns always the same DLPtr object for a given memory
 * pointer. Problems arise when one of the two following situations are met:
 * * we reference a memory zone inside another memory zone (for instance,
 *   an array element or a structure field). In that case, the DLPtr object
 *   must not free the allocated zone.
 * * two Typelib objects reference the same memory zone (first array
 *   element or first field of a structure). In that case, we must reuse the
 *   same DLPtr object, or DL will override the free function of the other
 *   DLPtr object -- which is obviously wrong, but nevertheless done.
 */
VALUE cxx2rb::value_wrap(Value v, VALUE registry, VALUE parent) {
    VALUE type = type_wrap(v.getType(), registry);
#ifdef VERBOSE
    Value parent_value = rb2cxx::object<Value>(parent);
    fprintf(
        stderr,
        "wrapping Typelib::Value %p from C++, type=%s, parent=%p and root=%p\n",
        v.getData(), v.getType().getName().c_str(), parent_value.getData(),
        value_root_ptr(parent));
#endif
    VALUE ptr = memory_wrap(v.getData(), false, value_root_ptr(parent));
    VALUE wrapper = rb_funcall(type, rb_intern("wrap"), 1, ptr);

    rb_iv_set(wrapper, "@parent", parent);
    rb_iv_set(wrapper, "@__typelib_invalidated", Qfalse);
    return wrapper;
}

static VALUE value_allocate(Type const &type, VALUE registry) {
    VALUE rb_type = cxx2rb::type_wrap(type, registry);
#ifdef VERBOSE
    fprintf(stderr, "allocating new value of type %s\n",
            type.getName().c_str());
#endif
    VALUE ptr = memory_allocate(type.getSize());
    memory_init(ptr, rb_type);
    VALUE wrapper = rb_funcall(rb_type, rb_intern("wrap"), 1, ptr);
    return wrapper;
}

namespace typelib_ruby {
VALUE cType = Qnil;
VALUE cNumeric = Qnil;
VALUE cOpaque = Qnil;
VALUE cNull = Qnil;
VALUE cIndirect = Qnil;
VALUE cPointer = Qnil;
VALUE cArray = Qnil;
VALUE cCompound = Qnil;
VALUE cEnum = Qnil;
VALUE cContainer = Qnil;
}

VALUE cxx2rb::class_of(Typelib::Type const &type) {
    using Typelib::Type;
    switch (type.getCategory()) {
    case Type::Numeric:
        return cNumeric;
    case Type::Compound:
        return cCompound;
    case Type::Pointer:
        return cPointer;
    case Type::Array:
        return cArray;
    case Type::Enum:
        return cEnum;
    case Type::Container:
        return cContainer;
    case Type::Opaque:
        return cOpaque;
    case Type::NullType:
        return cNull;
    default:
        return cType;
    }
}

VALUE cxx2rb::type_wrap(Type const &type, VALUE registry) {
    // Type objects are unique, we can register Ruby wrappers based
    // on the type pointer (instead of using names)
    WrapperMap &wrappers = rb2cxx::object<RbRegistry>(registry).wrappers;

    WrapperMap::const_iterator it = wrappers.find(&type);
    if (it != wrappers.end())
        return it->second.second;

    VALUE base = class_of(type);
    VALUE klass = rb_funcall(rb_cClass, rb_intern("new"), 1, base);
    VALUE rb_type =
        Data_Wrap_Struct(rb_cObject, 0, 0, const_cast<Type *>(&type));
    rb_iv_set(klass, "@registry", registry);
    rb_iv_set(klass, "@type", rb_type);
    rb_iv_set(klass, "@name", rb_str_new2(type.getName().c_str()));
    rb_iv_set(klass, "@null",
              (type.getCategory() == Type::NullType) ? Qtrue : Qfalse);
    rb_iv_set(klass, "@opaque",
              (type.getCategory() == Type::Opaque) ? Qtrue : Qfalse);
    rb_iv_set(klass, "@metadata", cxx2rb::metadata_wrap(type.getMetaData()));

    if (rb_respond_to(klass, rb_intern("subclass_initialize")))
        rb_funcall(klass, rb_intern("subclass_initialize"), 0);

    wrappers.insert(std::make_pair(&type, std::make_pair(false, klass)));
    return klass;
}

/**********************************************
 * Typelib::Type
 */

/* @overload type.to_csv([basename [, separator]])
 *
 * Returns a one-line representation of this type, using +separator+
 * to separate each fields. If +basename+ is given, use it as a
 * 'variable name'. For instance, calling this method on an array
 * with a basename of 'array' will return
 *
 *   array[0] array[1]
 *
 * without basename, it would be
 *
 *   [0] [1]
 *
 */
static VALUE type_to_csv(int argc, VALUE *argv, VALUE rbself) {
    VALUE basename = Qnil;
    VALUE separator = Qnil;
    rb_scan_args(argc, argv, "02", &basename, &separator);

    std::string bname = "", sep = " ";
    if (!NIL_P(basename))
        bname = StringValuePtr(basename);
    if (!NIL_P(separator))
        sep = StringValuePtr(separator);

    Type const &self(rb2cxx::object<Type>(rbself));
    std::ostringstream stream;
    stream << csv_header(self, bname, sep);
    std::string str = stream.str();
    return rb_str_new(str.c_str(), str.length());
}

/* @overload t.basename
 *
 * Returns the type name of the receiver with the namespace part
 * removed
 *
 * @return [String]
 */
static VALUE typelib_do_basename(VALUE mod, VALUE name) {
    std::string result = Typelib::getTypename(StringValuePtr(name));
    return rb_str_new(result.c_str(), result.length());
}

/* Internal helper method for Type#namespace */
static VALUE typelib_do_namespace(VALUE mod, VALUE name) {
    std::string result = Typelib::getNamespace(StringValuePtr(name));
    return rb_str_new(result.c_str(), result.length());
}

/* @overload split_typename
 *
 * Splits the typename of self into its components
 *
 * @return [Array<String>]
 */
static VALUE typelib_do_split_name(VALUE mod, VALUE name) {
    std::list<std::string> splitted =
        Typelib::splitTypename(StringValuePtr(name));
    VALUE result = rb_ary_new();
    for (std::list<std::string>::const_iterator it = splitted.begin();
         it != splitted.end(); ++it)
        rb_ary_push(result, rb_str_new(it->c_str(), it->length()));
    return result;
}

/* @overload t1 == t2
 *
 * Returns true if +t1+ and +t2+ are the same type definition.
 *
 * @return [Boolean]
 */
static VALUE type_equal_operator(VALUE rbself, VALUE rbwith) {
    if (rbself == rbwith)
        return Qtrue;
    if (!rb_obj_is_kind_of(rbwith, rb_cClass))
        return Qfalse;
    if ((rbwith == cType) ||
        !RTEST(rb_funcall(rbwith, rb_intern("<"), 1, cType)))
        return Qfalse;

    Type const &self(rb2cxx::object<Type>(rbself));
    Type const &with(rb2cxx::object<Type>(rbwith));
    bool result = (self == with) || self.isSame(with);
    return result ? Qtrue : Qfalse;
}

/* @overload type.size
 *
 * Returns the size in bytes of instances of +type+
 */
static VALUE type_size(VALUE self) {
    Type const &type(rb2cxx::object<Type>(self));
    return INT2FIX(type.getSize());
}

/* Returns the set of Type subclasses that represent the types needed to build
 * +type+.
 */
static VALUE type_dependencies(VALUE self) {
    Type const &type(rb2cxx::object<Type>(self));

    typedef std::set<Type const *> TypeSet;
    TypeSet dependencies = type.dependsOn();
    VALUE registry = type_get_registry(self);

    VALUE result = rb_ary_new();
    for (TypeSet::const_iterator it = dependencies.begin();
         it != dependencies.end(); ++it)
        rb_ary_push(result, cxx2rb::type_wrap(**it, registry));
    return result;
}

/* @overload type.casts_to?(other_type) => true or false
 *
 * Returns true if a value that is described by +type+ can be manipulated using
 * +other_type+. This is a weak form of equality
 */
static VALUE type_can_cast_to(VALUE self, VALUE to) {
    Type const &from_type(rb2cxx::object<Type>(self));
    Type const &to_type(rb2cxx::object<Type>(to));
    return from_type.canCastTo(to_type) ? Qtrue : Qfalse;
}

/*
 *  type.do_memory_layout(VALUE accept_pointers, VALUE accept_opaques, VALUE
 *merge_skip_copy, VALUE remove_trailing_skips) => [operations]
 *
 * Returns a representation of the MemoryLayout for this type. If
 * +with_pointers+ is true, then pointers will be included in the layout.
 * Otherwise, an exception is raised if pointers are part of the type
 */
static VALUE type_memory_layout(VALUE self, VALUE pointers, VALUE opaques,
                                VALUE merge, VALUE remove_trailing_skips) {
    Type const &type(rb2cxx::object<Type>(self));
    VALUE registry = type_get_registry(self);

    VALUE result = rb_ary_new();

    VALUE rb_memcpy = ID2SYM(rb_intern("FLAG_MEMCPY"));
    VALUE rb_skip = ID2SYM(rb_intern("FLAG_SKIP"));
    VALUE rb_array = ID2SYM(rb_intern("FLAG_ARRAY"));
    VALUE rb_end = ID2SYM(rb_intern("FLAG_END"));
    VALUE rb_container = ID2SYM(rb_intern("FLAG_CONTAINER"));

    try {
        MemoryLayout layout =
            Typelib::layout_of(type, RTEST(pointers), RTEST(opaques),
                               RTEST(merge), RTEST(remove_trailing_skips));

        // Now, convert into something representable in Ruby
        for (MemoryLayout::const_iterator it = layout.begin();
             it != layout.end(); ++it) {
            switch (*it) {
            case MemLayout::FLAG_MEMCPY:
                rb_ary_push(result, rb_memcpy);
                rb_ary_push(result, LONG2NUM(*(++it)));
                break;
            case MemLayout::FLAG_SKIP:
                rb_ary_push(result, rb_skip);
                rb_ary_push(result, LONG2NUM(*(++it)));
                break;
            case MemLayout::FLAG_ARRAY:
                rb_ary_push(result, rb_array);
                rb_ary_push(result, LONG2NUM(*(++it)));
                break;
            case MemLayout::FLAG_END:
                rb_ary_push(result, rb_end);
                break;
            case MemLayout::FLAG_CONTAINER:
                rb_ary_push(result, rb_container);
                rb_ary_push(result, cxx2rb::type_wrap(
                                        *reinterpret_cast<Container *>(*(++it)),
                                        registry));
                break;
            default:
                rb_raise(rb_eArgError,
                         "error encountered while parsing memory layout");
            }
        }

    } catch (std::exception const &e) {
        rb_raise(rb_eArgError, "%s", e.what());
    }

    return result;
}

/* PODs are assignable, pointers are dereferenced */
static VALUE type_is_assignable(Type const &type) {
    switch (type.getCategory()) {
    case Type::Numeric:
        return INT2FIX(1);
    case Type::Pointer:
        return type_is_assignable(
            dynamic_cast<Pointer const &>(type).getIndirection());
    case Type::Enum:
        return INT2FIX(1);
    default:
        return INT2FIX(0);
    }
    // never reached
}

VALUE typelib_ruby::type_get_registry(VALUE self) {
    return rb_iv_get(self, "@registry");
}

/***********************************************************************************
 *
 * Wrapping of the Value class
 *
 */

static void value_delete(void *self) { delete reinterpret_cast<Value *>(self); }

static VALUE value_alloc(VALUE klass) {
    return Data_Wrap_Struct(klass, 0, value_delete, new Value);
}

/** Allocates a new Typelib object, which wraps an untyped, unallocated Value
 * object
 */
static VALUE value_create_empty(VALUE klass) {
    Type const &t(rb2cxx::object<Type>(klass));
    VALUE value = Data_Wrap_Struct(klass, 0, value_delete, new Value(NULL, t));
    rb_iv_set(value, "@parent", Qnil);
    return value;
}

static void value_call_typelib_initialize(VALUE obj) {
    rb_iv_set(obj, "@__typelib_invalidated", Qfalse);
    rb_iv_set(obj, "@parent", Qnil);
    rb_funcall(obj, rb_intern("typelib_initialize"), 0);
}

/* @overload from_memory_zone(ptr)
 *
 * @param [Typelib::MemoryZone] ptr
 * @return [Typelib::Type]
 *
 * Allocates a new Typelib object that uses a given MemoryZone object
 */
static VALUE value_from_memory_zone(VALUE klass, VALUE ptr) {
    VALUE result = value_create_empty(klass);
    Value &value = rb2cxx::object<Value>(result);

    // Protect the memory zone against GC until we don't need it anymore
    rb_iv_set(result, "@ptr", ptr);
    value = Value(memory_cptr(ptr), value.getType());
#ifdef VERBOSE
    fprintf(stderr, "object %llu wraps memory zone %p\n",
            NUM2ULL(rb_obj_id(result)), value.getData());
#endif
    value_call_typelib_initialize(result);
    return result;
}

/* @overload new
 *
 * @return [Typelib::Type]
 *
 * Allocates a new Typelib object that has a freshly initialized buffer inside
 */
static VALUE value_new(VALUE klass) {
    Type const &type(rb2cxx::object<Type>(klass));
#ifdef VERBOSE
    fprintf(stderr, "allocating new value of type %s\n",
            type.getName().c_str());
#endif
    VALUE buffer = memory_allocate(type.getSize());
    memory_init(buffer, klass);

    return value_from_memory_zone(klass, buffer);
}

static VALUE value_do_from_buffer(VALUE rbself, VALUE string, VALUE pointers,
                                  VALUE opaques, VALUE merge,
                                  VALUE remove_trailing_skips) {
    Value value = rb2cxx::object<Value>(rbself);

    MemoryLayout layout =
        Typelib::layout_of(value.getType(), RTEST(pointers), RTEST(opaques),
                           RTEST(merge), RTEST(remove_trailing_skips));

    char *ruby_buffer = StringValuePtr(string);
    vector<uint8_t> cxx_buffer(ruby_buffer, ruby_buffer + RSTRING_LEN(string));
    try {
        Typelib::load(value, cxx_buffer, layout);
    } catch (std::exception const &e) {
        rb_raise(rb_eArgError, "%s", e.what());
    }
    return rbself;
}

/* @overload from_address(address)
 *
 * Creates a value that wraps a given memory address
 *
 * @param [Integer] address
 * @return [Typelib::Type]
 */
static VALUE value_from_address(VALUE klass, VALUE address) {
    VALUE result = value_create_empty(klass);
    Value &value = rb2cxx::object<Value>(result);
    value = Value(reinterpret_cast<void *>(NUM2ULL(address)), value.getType());
    rb_iv_set(result, "@ptr", memory_wrap(value.getData(), false, NULL));
    value_call_typelib_initialize(result);
    return result;
}

/* @overload address
 *
 * @return [Integer] returns the address of the memory zone this value wraps
 */
static VALUE value_address(VALUE self) {
    Value value = rb2cxx::object<Value>(self);
    return LONG2NUM((long)value.getData());
}

/* @overload endian_swap
 *
 * Creates a new value with an endianness opposite of the one of self
 *
 * @return [Typelib::Type] a new value whose endianness has been swapped
 * @see endian_swap!
 */
static VALUE value_endian_swap(VALUE self) {
    Value &value = rb2cxx::object<Value>(self);
    CompileEndianSwapVisitor compiled;
    compiled.apply(value.getType());

    VALUE registry = value_get_registry(self);
    VALUE result = value_allocate(value.getType(), registry);
    compiled.swap(value, rb2cxx::object<Value>(result));
    return result;
}

/* @overload endian_swap!
 *
 * Swaps the endianness of self
 *
 * @return [Typelib::Type] self
 * @see endian_swap
 */
static VALUE value_endian_swap_b(VALUE self, VALUE rb_compile) {
    Value &value = rb2cxx::object<Value>(self);
    endian_swap(value);
    return self;
}

static VALUE value_do_cast(VALUE self, VALUE target_type) {
    Value &value = rb2cxx::object<Value>(self);
    Type const &to_type(rb2cxx::object<Type>(target_type));

    if (value.getType() == to_type)
        return self;

    VALUE registry = rb_iv_get(target_type, "@registry");
    Value casted(value.getData(), to_type);
#ifdef VERBOSE
    fprintf(stderr, "wrapping casted value\n");
#endif

    return cxx2rb::value_wrap(casted, registry, self);
}

static VALUE value_invalidate(VALUE self) {
    if (NIL_P(rb_iv_get(self, "@parent")))
        rb_raise(rb_eArgError, "cannot invalidate a toplevel value");

    Value &value = rb2cxx::object<Value>(self);
#ifdef VERBOSE
    fprintf(stderr, "invalidating %llu, ptr=%p\n", NUM2ULL(rb_obj_id(self)),
            value.getData());
#endif
    value = Value(0, value.getType());
    rb_funcall(rb_iv_get(self, "@ptr"), rb_intern("invalidate"), 0);
    return Qnil;
}

static VALUE value_do_byte_array(VALUE self, VALUE pointers, VALUE opaques,
                                 VALUE merge, VALUE remove_trailing_skips) {
    Value &value = rb2cxx::object<Value>(self);
    MemoryLayout layout =
        Typelib::layout_of(value.getType(), RTEST(pointers), RTEST(opaques),
                           RTEST(merge), RTEST(remove_trailing_skips));

    vector<uint8_t> buffer;
    Typelib::dump(value, buffer, layout);
    return rb_str_new(reinterpret_cast<char *>(&buffer[0]), buffer.size());
}

/* call-seq:
 *  obj.marshalling_size => integer
 *
 * Returns the size of this value once marshalled by Typelib, i.e. the size of
 * the byte array returned by #to_byte_array
 */
static VALUE value_marshalling_size(VALUE self) {
    Value &value = rb2cxx::object<Value>(self);
    try {
        return INT2NUM(Typelib::getDumpSize(value));
    } catch (Typelib::NoLayout) {
        return Qnil;
    }
}

VALUE value_memory_eql_p(VALUE rbself, VALUE rbwith) {
    Value &self = rb2cxx::object<Value>(rbself);
    Value &with = rb2cxx::object<Value>(rbwith);

    if (self.getData() == with.getData())
        return Qtrue;

    // Type#== checks for type equality before calling memory_equal?
    Type const &type = self.getType();
    return memcmp(self.getData(), with.getData(), type.getSize()) == 0 ? Qtrue
                                                                       : Qfalse;
}

VALUE typelib_ruby::value_get_registry(VALUE self) {
    VALUE type = rb_funcall(self, rb_intern("class"), 0);
    return rb_iv_get(type, "@registry");
}

/**
 * call-seq:
 *   value.to_csv([separator])	    => string
 *
 * Returns a one-line representation of this value, using +separator+
 * to separate each fields
 */
static VALUE value_to_csv(int argc, VALUE *argv, VALUE self) {
    VALUE separator = Qnil;
    rb_scan_args(argc, argv, "01", &separator);

    Value const &value(rb2cxx::object<Value>(self));
    std::string sep = " ";
    if (!NIL_P(separator))
        sep = StringValuePtr(separator);

    std::ostringstream stream;
    stream << csv(value.getType(), value.getData(), sep);
    std::string str = stream.str();
    return rb_str_new(str.c_str(), str.length());
}

/* Initializes the memory to 0 */
static VALUE value_zero(VALUE self) {
    Value const &value(rb2cxx::object<Value>(self));
    Typelib::zero(value);
    return self;
}

static VALUE typelib_do_copy(VALUE, VALUE to, VALUE from) {
    Value v_from = rb2cxx::object<Value>(from);
    Value v_to = rb2cxx::object<Value>(to);

    if (v_from.getType() != v_to.getType()) {
        // Do a deep check for type equality
        if (!v_from.getType().canCastTo(v_to.getType()))
            rb_raise(rb_eArgError, "cannot copy: types are not compatible");
    }
    Typelib::copy(v_to.getData(), v_from.getData(), v_from.getType());
    return to;
}

/* call-seq:
 *  Typelib.compare(to, from) => true or false
 *
 * Proper comparison of two values. +to+ and +from+'s types do not have to be of
 * the same registries, as long as the types can be cast'ed into each other.
 */
static VALUE typelib_compare(VALUE, VALUE to, VALUE from) {
    Value v_from = rb2cxx::object<Value>(from);
    Value v_to = rb2cxx::object<Value>(to);

    if (v_from.getType() != v_to.getType()) {
        // Do a deep check for type equality
        if (!v_from.getType().canCastTo(v_to.getType()))
            rb_raise(rb_eArgError,
                     "cannot compare: %s and %s are not compatible types",
                     v_from.getType().getName().c_str(),
                     v_to.getType().getName().c_str());
    }
    try {
        bool result = Typelib::compare(v_to.getData(), v_from.getData(),
                                       v_from.getType());
        return result ? Qtrue : Qfalse;
    } catch (std::exception const &e) {
        rb_raise(rb_eArgError, "%s", e.what());
    }
}

void typelib_ruby::Typelib_init_values() {
    VALUE mTypelib = rb_define_module("Typelib");
    rb_define_singleton_method(mTypelib, "do_copy",
                               RUBY_METHOD_FUNC(typelib_do_copy), 2);
    rb_define_singleton_method(mTypelib, "compare",
                               RUBY_METHOD_FUNC(typelib_compare), 2);

    cType = rb_define_class_under(mTypelib, "Type", rb_cObject);
    rb_define_alloc_func(cType, value_alloc);
    rb_define_singleton_method(cType, "==",
                               RUBY_METHOD_FUNC(type_equal_operator), 1);
    rb_define_singleton_method(cType, "size", RUBY_METHOD_FUNC(&type_size), 0);
    rb_define_singleton_method(cType, "do_memory_layout",
                               RUBY_METHOD_FUNC(&type_memory_layout), 4);
    rb_define_singleton_method(cType, "do_dependencies",
                               RUBY_METHOD_FUNC(&type_dependencies), 0);
    rb_define_singleton_method(cType, "casts_to?",
                               RUBY_METHOD_FUNC(&type_can_cast_to), 1);
    rb_define_singleton_method(cType, "value_new", RUBY_METHOD_FUNC(&value_new),
                               0);
    rb_define_singleton_method(cType, "from_memory_zone",
                               RUBY_METHOD_FUNC(&value_from_memory_zone), 1);
    rb_define_singleton_method(cType, "from_address",
                               RUBY_METHOD_FUNC(&value_from_address), 1);

    rb_define_method(cType, "do_from_buffer",
                     RUBY_METHOD_FUNC(&value_do_from_buffer), 5);
    rb_define_method(cType, "zero!", RUBY_METHOD_FUNC(&value_zero), 0);
    rb_define_method(cType, "memory_eql?",
                     RUBY_METHOD_FUNC(&value_memory_eql_p), 1);
    rb_define_method(cType, "endian_swap", RUBY_METHOD_FUNC(&value_endian_swap),
                     0);
    rb_define_method(cType, "endian_swap!",
                     RUBY_METHOD_FUNC(&value_endian_swap_b), 0);
    rb_define_method(cType, "zone_address", RUBY_METHOD_FUNC(&value_address),
                     0);
    rb_define_method(cType, "do_cast", RUBY_METHOD_FUNC(&value_do_cast), 1);
    rb_define_method(cType, "do_invalidate",
                     RUBY_METHOD_FUNC(&value_invalidate), 0);

    rb_define_singleton_method(mTypelib, "do_basename",
                               RUBY_METHOD_FUNC(typelib_do_basename), 1);
    rb_define_singleton_method(mTypelib, "do_namespace",
                               RUBY_METHOD_FUNC(typelib_do_namespace), 1);
    rb_define_singleton_method(mTypelib, "split_typename",
                               RUBY_METHOD_FUNC(typelib_do_split_name), 1);

    rb_define_singleton_method(cType, "to_csv", RUBY_METHOD_FUNC(type_to_csv),
                               -1);
    rb_define_method(cType, "to_csv", RUBY_METHOD_FUNC(value_to_csv), -1);
    rb_define_method(cType, "do_byte_array",
                     RUBY_METHOD_FUNC(value_do_byte_array), 4);
    rb_define_method(cType, "marshalling_size",
                     RUBY_METHOD_FUNC(value_marshalling_size), 0);

    Typelib_init_specialized_types();
}
