#include "typelib.hh"
#include <string>

using namespace Typelib;
using std::string;
using namespace typelib_ruby;

static bool is_string_handler(Registry const& registry, Type const& type, bool known_size = false)
{
    if (type.getCategory() != Type::Array && type.getCategory() != Type::Pointer)
        return false;

    Type const* char_type(registry.get("/char"));
    if (!char_type)
        return false;

    Type const& data_type(static_cast<Indirect const&>(type).getIndirection());
    if (data_type.getName() != char_type->getName())
        return false;

    if (known_size && type.getCategory() == Type::Pointer)
        return false;

    return true;

}
static void string_buffer_get(Value const& value, char*& buffer, string::size_type& size)
{
    Type const& type(value.getType());
    if (type.getCategory() == Type::Array)
    {
        buffer = reinterpret_cast<char*>(value.getData());
        size = static_cast<Array const&>(type).getDimension();
    }
    else
    {
        buffer = *reinterpret_cast<char**>(value.getData());
        size   = string::npos;
    }
}

static VALUE value_string_handler_p(VALUE self)
{
    Value const& value(rb2cxx::object<Value>(self));
    Type  const& type(value.getType());
    Registry const& registry = rb2cxx::object<Registry>(value_get_registry(self));
    return is_string_handler(registry, type) ? Qtrue : Qfalse;
}

/*
 * Handle convertion between Ruby's String and C 'char*' types
 * It is a module function used to define #to_str on the relevant types
 * NEVER call it directly
 */
static VALUE value_from_string(VALUE mod, VALUE self, VALUE from, VALUE known_good_type)
{
    Value const& value(rb2cxx::object<Value>(self));
    Type  const& type(value.getType());
    Registry const& registry = rb2cxx::object<Registry>(value_get_registry(self));

    if (!RTEST(known_good_type) && !is_string_handler(registry, type, true))
        rb_raise(rb_eTypeError, "Ruby strings can only be converted to char arrays");

    char * buffer;
    string::size_type buffer_size;
    string_buffer_get(value, buffer, buffer_size);

    string::size_type from_length = RSTRING_LEN(StringValue(from));
    if ((buffer_size - 1) < from_length)
        rb_raise(rb_eArgError, "array to small: %lu, while %lu was needed", buffer_size, from_length + 1);

    strncpy(buffer, StringValueCStr(from), buffer_size);
    buffer[buffer_size - 1] = 0;
    return self;
}

/** Converts a C string to a Ruby string.
 * NEVER call that directly. It is used to define #to_str on the relevant
 * instances of Type
 */
static VALUE value_to_string(VALUE mod, VALUE self, VALUE known_good_type)
{
    Value const& value(rb2cxx::object<Value>(self));
    Type  const& type(value.getType());
    Registry const& registry = rb2cxx::object<Registry>(value_get_registry(self));

    if (!RTEST(known_good_type) && !is_string_handler(registry, type))
        rb_raise(rb_eRuntimeError, "invalid conversion to string");

    char* buffer;
    string::size_type buffer_size;
    string_buffer_get(value, buffer, buffer_size);

    if (buffer_size == string::npos)
        return rb_str_new2(buffer);
    else
    {
        // Search the real end of the string inside the buffer
        string::size_type real_size;
        for (real_size = 0; real_size < buffer_size; ++real_size)
        {
            if (!buffer[real_size])
                break;
        }
        return rb_str_new(buffer, real_size);
    }
}

void typelib_ruby::Typelib_init_strings()
{
    rb_define_singleton_method(cType, "to_string",    RUBY_METHOD_FUNC(&value_to_string), 2);
    rb_define_singleton_method(cType, "from_string",  RUBY_METHOD_FUNC(&value_from_string), 3);
    rb_define_method(cType, "string_handler?", RUBY_METHOD_FUNC(&value_string_handler_p), 0);
}

