#include "typelib.hh"
#include <ruby/version.h>
#include <ruby/encoding.h>

#include <typelib/typemodel.hh>

using namespace Typelib;
using namespace typelib_ruby;

VALUE typelib_ruby::cMetaData;
rb_encoding* enc_utf8;

VALUE cxx2rb::metadata_wrap(MetaData& metadata)
{
    return Data_Wrap_Struct(cMetaData, 0, 0, &metadata);
}

static VALUE metadata_include_p(VALUE self, VALUE key)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    return metadata.include(StringValuePtr(key)) ? Qtrue : Qfalse;
}

static VALUE metadata_get(VALUE self, VALUE key)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    MetaData::Values v = metadata.get(StringValuePtr(key));

    VALUE result = rb_ary_new();
    for (MetaData::Values::const_iterator it = v.begin(); it != v.end(); ++it)
        rb_ary_push(result, rb_enc_str_new(it->c_str(), it->length(), enc_utf8));
    return result;
}

static VALUE metadata_keys(VALUE self)
{
    MetaData& metadata = rb2cxx::object<MetaData>(self);
    MetaData::Map const& map = metadata.get();

    VALUE result = rb_ary_new();
    for (MetaData::Map::const_iterator it = map.begin(); it != map.end(); ++it)
        rb_ary_push(result, rb_enc_str_new(it->first.c_str(), it->first.length(), enc_utf8));
    return result;
}

static VALUE metadata_add(int argc, VALUE* argv, VALUE self)
{
    VALUE key;
    VALUE values;
    rb_scan_args(argc, argv, "1*", &key, &values);

    MetaData& metadata = rb2cxx::object<MetaData>(self);
    std::string rb_key(StringValuePtr(key));

    MetaData::Values new_values;
    long length = RARRAY_LEN(values);
    for (long i = 0; i < length; ++i)
    {
        VALUE val = rb_ary_entry(values, i);
        new_values.insert(StringValuePtr(val));
    }
    metadata.add(rb_key, new_values);
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

    rb_define_method(cMetaData, "include?", RUBY_METHOD_FUNC(metadata_include_p), 1);
    rb_define_method(cMetaData, "get", RUBY_METHOD_FUNC(metadata_get), 1);
    rb_define_method(cMetaData, "add", RUBY_METHOD_FUNC(metadata_add), -1);
    rb_define_method(cMetaData, "clear", RUBY_METHOD_FUNC(metadata_clear), -1);
    rb_define_method(cMetaData, "keys", RUBY_METHOD_FUNC(metadata_keys), 0);
    enc_utf8 = rb_enc_find("utf-8");
}

