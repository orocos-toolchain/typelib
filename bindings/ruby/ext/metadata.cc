#include "typelib.hh"
#include <ruby/version.h>

#include <typelib/typemodel.hh>

using namespace Typelib;
using namespace typelib_ruby;

VALUE typelib_ruby::cMetaData;

VALUE cxx2rb::metadata_wrap(MetaData& metadata)
{
    return Data_Wrap_Struct(cMetaData, 0, 0, &metadata);
}

static VALUE metadata_get(VALUE self, VALUE key)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    MetaData::Values v = metadata.get(StringValuePtr(key));

    VALUE result = rb_ary_new();
    for (MetaData::Values::const_iterator it = v.begin(); it != v.end(); ++it)
        rb_ary_push(result, rb_str_new_cstr(it->c_str()));
    return result;
}

static VALUE metadata_keys(VALUE self)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    MetaData::Map const& map = metadata.get();

    VALUE result = rb_ary_new();
    for (MetaData::Map::const_iterator it = map.begin(); it != map.end(); ++it)
        rb_ary_push(result, rb_str_new_cstr(it->first.c_str()));
    return result;
}

static VALUE metadata_set(VALUE self, VALUE key, VALUE value)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    metadata.set(StringValuePtr(key), StringValuePtr(value));
    return Qnil;
}

static VALUE metadata_add(VALUE self, VALUE key, VALUE value)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    metadata.add(StringValuePtr(key), StringValuePtr(value));
    return Qnil;
}

static VALUE metadata_clear(int argc, VALUE* argv, VALUE self)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    if (argc == 0)
        metadata.clear();
    else if (argc == 1)
        metadata.clear(StringValuePtr(argv[0]));
    return Qnil;
}

static void metadata_free(void* metadata)
{
    delete reinterpret_cast<MetaData*>(metadata);
}

static VALUE metadata_alloc(VALUE klass)
{
    MetaData* metadata = new MetaData;
    return Data_Wrap_Struct(cMetaData, 0, metadata_free, metadata);
}

void typelib_ruby::Typelib_init_metadata()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    cMetaData   = rb_define_class_under(mTypelib, "MetaData", rb_cObject);
    rb_define_alloc_func(cMetaData, metadata_alloc);

    rb_define_method(cMetaData, "get", RUBY_METHOD_FUNC(metadata_get), 1);
    rb_define_method(cMetaData, "set", RUBY_METHOD_FUNC(metadata_set), 2);
    rb_define_method(cMetaData, "add", RUBY_METHOD_FUNC(metadata_add), 2);
    rb_define_method(cMetaData, "clear", RUBY_METHOD_FUNC(metadata_clear), -1);
    rb_define_method(cMetaData, "keys", RUBY_METHOD_FUNC(metadata_keys), 0);
}

