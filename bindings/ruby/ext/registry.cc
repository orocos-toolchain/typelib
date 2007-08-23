#include "typelib.hh"

#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/registryiterator.hh>
#include <utilmm/configfile/configset.hh>

using namespace Typelib;
using utilmm::config_set;
using std::string;

static VALUE cRegistry = Qnil;
namespace cxx2rb {
    template<> VALUE class_of<Registry>() { return cRegistry; }
}

/***********************************************************************************
 *
 * Wrapping of the Registry class
 *
 */


static 
void registry_free(void* ptr) { delete reinterpret_cast<Registry*>(ptr); }
static
void wrappers_mark(void* ptr)
{
    using cxx2rb::WrapperMap;
    WrapperMap const& wrappers = *reinterpret_cast<WrapperMap const*>(ptr);
    for (WrapperMap::const_iterator it = wrappers.begin(); it != wrappers.end(); ++it)
	rb_gc_mark(it->second);
}

static
void wrappers_free(void* ptr) { delete reinterpret_cast<cxx2rb::WrapperMap*>(ptr); }

static
VALUE registry_alloc(VALUE klass)
{
    Registry* registry = new Registry;
    VALUE rb_registry = Data_Wrap_Struct(klass, 0, registry_free, registry);

    cxx2rb::WrapperMap *wrappers = new cxx2rb::WrapperMap;
    VALUE rb_wrappers = Data_Wrap_Struct(rb_cObject, wrappers_mark, wrappers_free, wrappers);
    rb_iv_set(rb_registry, "@wrappers", rb_wrappers);
    return rb_registry;
}


static
VALUE registry_do_get(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.get( StringValuePtr(name) );

    if (! type) return Qnil;
    return cxx2rb::type_wrap(*type, self);
}

static
VALUE registry_do_build(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.build( StringValuePtr(name) );

    if (! type) 
        rb_raise(rb_eTypeError, "invalid type %s", StringValuePtr(name));
    return cxx2rb::type_wrap(*type, self);
}


/* call-seq:
 *  alias(new_name, name)	    => self
 *
 * Make +new_name+ refer to the type named +name+
 */
static
VALUE registry_alias(VALUE self, VALUE name, VALUE aliased)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    int error;
    try { 
	registry.alias(StringValuePtr(aliased), StringValuePtr(name)); 
	return self;
    }
    catch(BadName)   { error = 0; }
    catch(Undefined) { error = 1; }
    switch(error)
    {
	case 0: rb_raise(rb_eArgError, "invalid type name %s", StringValuePtr(name));
	case 1: rb_raise(rb_eArgError, "no such type %s", StringValuePtr(aliased));
    }

    // never reached
    assert(false);
}

/* Private method to import a given file in the registry
 * We expect Registry#import to format the arguments before calling
 * do_import
 */
static
VALUE registry_import(VALUE self, VALUE file, VALUE kind, VALUE merge, VALUE options)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    config_set config;
    for (int i = 0; i < RARRAY(options)->len; ++i)
    {
	VALUE entry = RARRAY(options)->ptr[i];
	VALUE k = RARRAY(entry)->ptr[0];
	VALUE v = RARRAY(entry)->ptr[1];

	if ( TYPE(v) == T_ARRAY )
	{
	    for (int j = 0; j < RARRAY(v)->len; ++j)
		config.insert(StringValuePtr(k), StringValuePtr( RARRAY(v)->ptr[j] ));
	}
	else if (TYPE(v) == T_TRUE || TYPE(v) == T_FALSE)
	    config.set(StringValuePtr(k), RTEST(v) ? "true" : "false");
	else
	    config.set(StringValuePtr(k), StringValuePtr(v));
    }
	
    
    std::string error_string;
    try { 
	if (RTEST(merge))
	{
	    Registry temp;
	    PluginManager::load(StringValuePtr(kind), StringValuePtr(file), config, temp); 
	    registry.merge(temp);
	}
	else
	    PluginManager::load(StringValuePtr(kind), StringValuePtr(file), config, registry); 
	return Qnil;
    }
    catch(Typelib::ImportError& e) { error_string = e.what(); }
    catch(Typelib::RegistryException const& e) { error_string = e.toString(); }

    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}

/*
 * call-seq:
 *   merge(other_registry) => self
 *
 * Move all types found in +other_registry+ into +self+
 */
static
VALUE registry_merge(VALUE self, VALUE rb_merged)
{
    std::string error_string;

    Registry& registry = rb2cxx::object<Registry>(self);
    Registry& merged   = rb2cxx::object<Registry>(rb_merged);
    try { 
	registry.merge(merged);
	return self;
    }
    catch(Typelib::ImportError& e) { error_string = e.what(); }
    catch(Typelib::RegistryException const& e) { error_string = e.toString(); }

    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}

/*
 * each_type(include_alias = false) { |type| ... }
 *
 * Iterates on the types found in this registry. If include_alias is true, also
 * yield the aliased types.
 */
static
VALUE registry_each_type(int argc, VALUE* argv, VALUE self)
{
    VALUE rb_include_alias;
    rb_scan_args(argc, argv, "01", &rb_include_alias);
    bool include_alias = RTEST(rb_include_alias);

    Registry& registry = rb2cxx::object<Registry>(self);
    std::string error_string;

    try 
    {
	RegistryIterator it = registry.begin();
	RegistryIterator end = registry.end();
	if (include_alias)
	{
	    for (; it != end; ++it)
	    {
		std::string const& type_name = it.getName();
		VALUE rb_type_name = rb_str_new(type_name.c_str(), type_name.length());
		rb_yield_values(2, rb_type_name, cxx2rb::type_wrap(*it, self));
	    }
	}
	else
	{
	    for (; it != end; ++it)
	    {
		if (!it.isAlias())
		    rb_yield(cxx2rb::type_wrap(*it, self));
	    }
	}

	return self;
    }
    catch(Typelib::RegistryException const& e) { error_string = e.toString(); }
    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}

/* Export the given registry into xml
 */
static
VALUE registry_to_xml(VALUE self)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    
    std::string as_xml = PluginManager::save("tlb", registry);
    return rb_str_new(as_xml.c_str(), as_xml.length());
}

static
VALUE registry_from_xml(VALUE mod, VALUE xml)
{
    VALUE rb_registry = rb_funcall(cRegistry, rb_intern("new"), 0);
    Registry& registry = rb2cxx::object<Registry>(rb_registry);

    std::istringstream istream(StringValuePtr(xml));
    config_set config;
    PluginManager::load("tlb", istream, config, registry);

    return rb_registry;
}

void Typelib_init_registry()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_do_get), 1);
    rb_define_method(cRegistry, "build", RUBY_METHOD_FUNC(registry_do_build), 1);
    rb_define_method(cRegistry, "each_type", RUBY_METHOD_FUNC(registry_each_type), -1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 4);
    rb_define_method(cRegistry, "to_xml", RUBY_METHOD_FUNC(registry_to_xml), 0);
    rb_define_singleton_method(cRegistry, "from_xml", RUBY_METHOD_FUNC(registry_from_xml), 1);
    rb_define_method(cRegistry, "alias", RUBY_METHOD_FUNC(registry_alias), 2);
    rb_define_method(cRegistry, "merge", RUBY_METHOD_FUNC(registry_merge), 1);
}

