#include "typelib.hh"

#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/exporter.hh>
#include <typelib/registryiterator.hh>
#include <lang/csupport/standard_types.hh>
#include <utilmm/configfile/configset.hh>
#include <set>

using namespace Typelib;
using utilmm::config_set;
using namespace std;
using namespace typelib_ruby;
using cxx2rb::RbRegistry;


namespace typelib_ruby {
    VALUE cRegistry = Qnil;
    VALUE eNotFound = Qnil;
}

/***********************************************************************************
 *
 * Wrapping of the Registry class
 *
 */


static 
void registry_free(void* ptr)
{
    using cxx2rb::WrapperMap;
    RbRegistry* rbregistry = reinterpret_cast<RbRegistry*>(ptr);
    WrapperMap& wrappers = rbregistry->wrappers;
    for (WrapperMap::iterator it = wrappers.begin(); it != wrappers.end(); ++it)
    {
        if (it->second.first)
            delete it->first;
    }

    delete rbregistry;
}

static
void registry_mark(void* ptr)
{
    using cxx2rb::WrapperMap;
    WrapperMap const& wrappers = reinterpret_cast<RbRegistry const*>(ptr)->wrappers;
    for (WrapperMap::const_iterator it = wrappers.begin(); it != wrappers.end(); ++it)
	rb_gc_mark(it->second.second);
}

static
VALUE registry_wrap(VALUE klass, Registry* registry)
{
    return Data_Wrap_Struct(klass, registry_mark, registry_free, new RbRegistry(registry));
}

static
VALUE registry_alloc(VALUE klass)
{
    Registry* registry = new Registry;
    return registry_wrap(klass, registry);
}


static
VALUE registry_includes_p(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.get( StringValuePtr(name) );
    return type ? Qtrue : Qfalse;
}

/* call-seq:
 *   registry.remove(type) => removed_types
 *
 * Removes +type+ from the registry, along with all the types that depends on
 * +type+.
 *
 * Returns the set of removed types
 */
static
VALUE registry_remove(VALUE self, VALUE rbtype)
{
    RbRegistry& rbregistry = rb2cxx::object<RbRegistry>(self);
    Registry&   registry = *(rbregistry.registry);
    Type const& type(rb2cxx::object<Type>(rbtype));
    std::set<Type*> deleted = registry.remove(type);
    
    VALUE result = rb_ary_new();
    std::set<Type*>::iterator it, end;
    for (it = deleted.begin(); it != deleted.end(); ++it)
    {
        rb_ary_push(result, cxx2rb::type_wrap(**it, self));
        cxx2rb::WrapperMap::iterator wrapper_it = rbregistry.wrappers.find(*it);
        wrapper_it->second.first = true;
    }

    return result;
}

/* call-seq:
 *   registry.source_id_of(type) => source_or_nil
 *
 * Returns the source ID for the given type, if one is set
 */
static
VALUE registry_source_id_of(VALUE self, VALUE rbtype)
{
    RbRegistry& rbregistry = rb2cxx::object<RbRegistry>(self);
    Registry&   registry = *(rbregistry.registry);
    Type const& type(rb2cxx::object<Type>(rbtype));

    RegistryIterator it = registry.find(type.getName());
    if (it == registry.end())
        rb_raise(rb_eArgError, "this registry has no type called %s", type.getName().c_str());

    std::string source = it.getSource();
    if (source.empty()) return Qnil;
    return rb_str_new(it.getSource().c_str(), it.getSource().length());
}

/* call-seq:
 *   registry.reverse_depends(type) => types
 *
 * Returns the array of types that depend on +type+, including +type+ itself
 */
static
VALUE registry_reverse_depends(VALUE self, VALUE rbtype)
{
    Registry const& registry = rb2cxx::object<Registry>(self);
    Type const& type(rb2cxx::object<Type>(rbtype));
    std::set<Type const*> rdeps = registry.reverseDepends(type);
    
    VALUE result = rb_ary_new();
    std::set<Type const*>::iterator it, end;
    for (it = rdeps.begin(); it != rdeps.end(); ++it)
        rb_ary_push(result, cxx2rb::type_wrap(**it, self));

    return result;
}

static
VALUE registry_do_get(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.get( StringValuePtr(name) );

    if (! type)
        rb_raise(eNotFound, "there is no type in this registry with the name '%s'", StringValuePtr(name));
    return cxx2rb::type_wrap(*type, self);
}

static
VALUE registry_do_build(int argc, VALUE* argv, VALUE self)
{
    VALUE name = argv[0];
    int size = 0;
    if (argc == 0 || argc > 2)
        rb_raise(rb_eArgError, "expected one or two arguments, got %i", argc);
    else if (argc == 2)
        size = NUM2INT(argv[1]);

    Registry& registry = rb2cxx::object<Registry>(self);
    try {
        Type const* type = registry.build( StringValuePtr(name), size );
        if (! type) 
            rb_raise(eNotFound, "cannot find %s in registry", StringValuePtr(name));
        return cxx2rb::type_wrap(*type, self);
    }
    catch(std::exception const& e) { rb_raise(rb_eRuntimeError, "%s", e.what()); }
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

    try { 
	registry.alias(StringValuePtr(aliased), StringValuePtr(name)); 
	return self;
    } catch(BadName) {
        rb_raise(rb_eArgError, "invalid type name %s", StringValuePtr(name));
    } catch(Undefined) {
        rb_raise(eNotFound, "there is not type in this registry with the name '%s'", StringValuePtr(aliased));
    }
}

static
void setup_configset_from_option_array(config_set& config, VALUE options)
{
    int options_length = RARRAY_LEN(options);
    for (int i = 0; i < options_length; ++i)
    {
	VALUE entry = RARRAY_PTR(options)[i];
	VALUE k = RARRAY_PTR(entry)[0];
	VALUE v = RARRAY_PTR(entry)[1];

	if ( rb_obj_is_kind_of(v, rb_cArray) )
	{
            VALUE first_el = rb_ary_entry(v, 0);
            if (rb_obj_is_kind_of(first_el, rb_cArray))
            {
                // We are building recursive config sets
                for (int j = 0; j < RARRAY_LEN(v); ++j)
                {
                    config_set* child = new config_set;
                    setup_configset_from_option_array(*child, rb_ary_entry(v, j));
                    config.insert(StringValuePtr(k), child);
                }
            }
            else
            {
                for (int j = 0; j < RARRAY_LEN(v); ++j)
                {
                    VALUE value = rb_ary_entry(v, j);
                    config.insert(StringValuePtr(k), StringValuePtr(value));
                }
            }
	}
	else if (TYPE(v) == T_TRUE || TYPE(v) == T_FALSE)
	    config.set(StringValuePtr(k), RTEST(v) ? "true" : "false");
	else
	    config.set(StringValuePtr(k), StringValuePtr(v));
    }
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
    setup_configset_from_option_array(config, options);
    
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
    catch(boost::bad_lexical_cast e)   { error_string = e.what(); }
    catch(std::exception const& e) { error_string = e.what(); }

    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}

/* Private method called by Registry#export. This latter method is supposed
 * to format the +options+ argument from a Ruby hash into an array suitable
 * for setup_configset_from_option_array
 */
static
VALUE registry_export(VALUE self, VALUE kind, VALUE options)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    config_set config;
    setup_configset_from_option_array(config, options);
    
    string error_message;
    try {
	std::string exported = PluginManager::save(StringValuePtr(kind), config, registry);
	return rb_str_new(exported.c_str(), exported.length());
    }
    catch (std::exception const& e) { error_message = e.what(); }
    rb_raise(rb_eRuntimeError, "%s", error_message.c_str());
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
    catch(std::exception const& e) { error_string = e.what(); }
    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}


/*
 * call-seq:
 *   resize(new_sizes) => nil
 *
 * Changes the size of some types, while modifying the types that depend on
 * them to keep the registry consistent.
 */
static
VALUE registry_resize(VALUE self, VALUE new_sizes)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    std::map<std::string, size_t> sizes;
    size_t map_size   = RARRAY_LEN(new_sizes);
    VALUE* map_values = RARRAY_PTR(new_sizes);
    for (size_t i = 0; i < map_size; ++i)
    {
        VALUE* pair = RARRAY_PTR(map_values[i]);
        sizes.insert(std::make_pair(
                StringValuePtr(pair[0]),
                NUM2INT(pair[1])));
    }
    try { 
	registry.resize(sizes);
	return Qnil;
    }
    catch(std::exception const& e) {
        rb_raise(rb_eRuntimeError, "%s", e.what());
    }
}

static
VALUE registry_minimal(VALUE self, VALUE rb_auto, VALUE with_aliases)
{
    std::string error_string;
    Registry& registry   = rb2cxx::object<Registry>(self);

    if (rb_obj_is_kind_of(rb_auto, rb_cString))
    {
        try { 
            Registry* result = registry.minimal(StringValuePtr(rb_auto), RTEST(with_aliases));
            return registry_wrap(cRegistry, result);
        }
        catch(std::exception const& e)
        { rb_raise(rb_eRuntimeError, "%s", e.what()); }
    }
    else
    {
        Registry& auto_types = rb2cxx::object<Registry>(rb_auto);
        try { 
            Registry* result = registry.minimal(auto_types);
            return registry_wrap(cRegistry, result);
        }
        catch(std::exception const& e)
        { rb_raise(rb_eRuntimeError, "%s", e.what()); }
    }
}


static void yield_types(VALUE self, bool with_aliases, RegistryIterator it, RegistryIterator end)
{
    if (with_aliases)
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
}

/* @overload each_type(prefix, false)
 *   Enumerates the types and not the aliases
 *
 *   @param [nil,String] prefix if non-nil, only types whose name is this prefix
 *     will be enumerated
 *   @yieldparam [Model<Typelib::Type>] type a type
 *
 * @overload each_type(prefix, true)
 *   Enumerates the types and the aliases
 *
 *   @param [nil,String] prefix if non-nil, only types whose name is this prefix
 *     will be enumerated
 *   @yieldparam [String] name the type name, it is different from type.name for
 *     aliases
 *   @yieldparam [Model<Typelib::Type>] type a type
 */
static
VALUE registry_each_type(VALUE self, VALUE filter_, VALUE with_aliases_)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    bool with_aliases = RTEST(with_aliases_);
    std::string filter;
    if (RTEST(filter_))
        filter = StringValuePtr(filter_);

    try 
    {
        if (filter.empty())
            yield_types(self, with_aliases, registry.begin(), registry.end());
        else
            yield_types(self, with_aliases, registry.begin(filter), registry.end(filter));

	return self;
    }
    catch(std::exception const& e)
    {
        rb_raise(rb_eRuntimeError, "%s", e.what());
    }
}

/* call-seq:
 *  registry.merge_xml(xml) => registry
 * 
 * Build a registry from a string, which is formatted as Typelib's own XML
 * format.  See also #export, #import and #to_xml
 */
static
VALUE registry_merge_xml(VALUE rb_registry, VALUE xml)
{
    Registry& registry = rb2cxx::object<Registry>(rb_registry);

    std::istringstream istream(StringValuePtr(xml));
    config_set config;
    try { PluginManager::load("tlb", istream, config, registry); }
    catch(boost::bad_lexical_cast e)
    { rb_raise(rb_eArgError, "cannot load xml: %s", e.what()); }
    catch(std::exception const& e)
    { rb_raise(rb_eArgError, "cannot load xml: %s", e.what()); }

    return rb_registry;
}

/*
 * call-seq:
 *  Registry.aliases_of(type) => list_of_names
 *
 * Lists all the known aliases for the given type
 */
static VALUE registry_aliases_of(VALUE self, VALUE type_)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const& type(rb2cxx::object<Type>(type_));
    std::set<std::string> aliases =
        registry.getAliasesOf(type);

    VALUE result = rb_ary_new();
    for (set<string>::const_iterator it = aliases.begin(); it != aliases.end(); ++it)
        rb_ary_push(result, rb_str_new(it->c_str(), it->length()));
    return result;
}

/*
 * call-seq:
 *  registry.clear_aliases
 *
 * Removes all aliases defined on this registry
 */
static VALUE registry_clear_aliases(VALUE self)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    registry.clearAliases();
    return Qnil;
}

/*
 * call-seq:
 *  registry.size => size
 *
 * Returns the number of types registered in +self+
 */
static VALUE registry_size(VALUE self)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    return INT2NUM(registry.size());
}

/*
 * call-seq:
 *  Registry.available_containers => container_names
 *
 * Returns the set of known container names
 */
static VALUE registry_available_container(VALUE registry_module)
{
    Typelib::Container::AvailableContainers containers =
        Typelib::Container::availableContainers();

    VALUE result = rb_ary_new();
    Typelib::Container::AvailableContainers::const_iterator
        it = containers.begin(),
        end = containers.end();

    while (it != end)
    {
        std::string name = it->first;
        rb_ary_push(result, rb_str_new(name.c_str(), name.length()));
        ++it;
    }
    return result;
}

/*
 * call-seq:
 *  registry.define_container(name, kind, element_type, size) => new_type
 *
 * Defines a new container instance with the given container kind and element
 * type. +element_type+ is a type object, i.e. a subclass of Type, that has to
 * be part of +registry+ (obtained through registry.get or registry.build). If
 * +element_type+ comes from another registry object, the method raises
 * ArgumentError.
 *
 * Moreover, the method raises NotFound if +kind+ is not a known container name.
 */
static VALUE registry_define_container(VALUE registry, VALUE kind, VALUE element, VALUE _size)
{
    Registry& reg = rb2cxx::object<Registry>(registry);
    Type const& element_type(rb2cxx::object<Type>(element));
    // Check that +reg+ contains +element_type+
    if (!reg.isIncluded(element_type))
        rb_raise(rb_eArgError, "the given type object comes from a different type registry");

    try {
        Container const& new_type = Container::createContainer(reg, StringValuePtr(kind), element_type);
        size_t size = NUM2INT(_size);
        if (size != 0)
            reg.get_(new_type).setSize(size);
        return cxx2rb::type_wrap(new_type, registry);
    } catch(Typelib::UnknownContainer) {
        rb_raise(eNotFound, "%s is not a known container type", StringValuePtr(kind));
    }
}

/*
 * call-seq:
 *  Registry.add_standard_cxx_types(registry) => registry
 *
 * Adds to +registry+  some standard C/C++ types that Typelib can represent
 */
static VALUE registry_add_standard_cxx_types(VALUE klass, VALUE registry)
{
    Registry& reg = rb2cxx::object<Registry>(registry);
    try { Typelib::CXX::addStandardTypes(reg); }
    catch(Typelib::AlreadyDefined e)
    { rb_raise(rb_eArgError, "%s", e.what()); }
    return registry;
}

/*
 * call-seq:
 *   registry.do_create_compound(name, field_defs, size)
 *
 * Creates a new compound type in which the fields are provided by +field_defs+
 */
static VALUE registry_create_compound(VALUE registry, VALUE name, VALUE field_defs, VALUE _size)
{
    Registry& reg = rb2cxx::object<Registry>(registry);

    std::auto_ptr<Typelib::Compound> new_t(new Typelib::Compound(StringValuePtr(name)));

    int field_count = RARRAY_LEN(field_defs);
    for (int i = 0; i < field_count; ++i)
    {
        VALUE field = rb_ary_entry(field_defs, i);

        VALUE rb_name = rb_ary_entry(field, 0);
        std::string field_name(StringValuePtr(rb_name));
        Typelib::Type& field_type(rb2cxx::object<Type>(rb_ary_entry(field, 1)));
        int offset(NUM2INT(rb_ary_entry(field, 2)));
        new_t->addField(field_name, field_type, offset);
    }

    Typelib::Compound* type = new_t.release();
    size_t size = NUM2INT(_size);
    if (size != 0)
        type->setSize(size);
    try { reg.add(type, true, ""); }
    catch(std::runtime_error e) { rb_raise(rb_eArgError, "%s", e.what()); }
    return cxx2rb::type_wrap(*type, registry);
}

/*
 * call-seq:
 *   registry.create_enum(name, symbol_defs, size)
 *
 * Creates a new compound type in which the fields are provided by +field_defs+
 */
static VALUE registry_create_enum(VALUE registry, VALUE name, VALUE symbol_defs, VALUE _size)
{
    Registry& reg = rb2cxx::object<Registry>(registry);

    std::auto_ptr<Typelib::Enum> new_t(new Typelib::Enum(StringValuePtr(name)));

    int symbol_count = RARRAY_LEN(symbol_defs);
    for (int i = 0; i < symbol_count; ++i)
    {
        VALUE sym = rb_ary_entry(symbol_defs, i);

        VALUE rb_name = rb_ary_entry(sym, 0);
        std::string sym_name(StringValuePtr(rb_name));
        int sym_value(NUM2INT(rb_ary_entry(sym, 1)));
        new_t->add(sym_name, sym_value);
    }

    Typelib::Enum* type = new_t.release();
    size_t size = NUM2INT(_size);
    if (size != 0) type->setSize(size);
    try { reg.add(type, true, ""); }
    catch(std::runtime_error e) { rb_raise(rb_eArgError, "%s", e.what()); }
    return cxx2rb::type_wrap(*type, registry);
}

/*
 * call-seq:
 *   registry.create_opaque(name, size)
 *
 * Creates a new opaque type with the given name and size
 */
static VALUE registry_create_opaque(VALUE registry, VALUE _name, VALUE _size)
{
    Registry& reg = rb2cxx::object<Registry>(registry);
    Typelib::Type* type = new Typelib::OpaqueType(StringValuePtr(_name), NUM2INT(_size));
    try { reg.add(type, true, ""); }
    catch(std::runtime_error e) { rb_raise(rb_eArgError, "%s", e.what()); }
    return cxx2rb::type_wrap(*type, registry);
}

/*
 * call-seq:
 *   registry.create_null(name)
 *
 * Creates a new null type with the given name
 */
static VALUE registry_create_null(VALUE registry, VALUE _name)
{
    Registry& reg = rb2cxx::object<Registry>(registry);
    Typelib::Type* type = new Typelib::NullType(StringValuePtr(_name));
    try { reg.add(type, true, ""); }
    catch(std::runtime_error e) { rb_raise(rb_eArgError, "%s", e.what()); }
    return cxx2rb::type_wrap(*type, registry);
}

void typelib_ruby::Typelib_init_registry()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    eNotFound = rb_define_class_under(mTypelib, "NotFound", rb_eRuntimeError);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "size", RUBY_METHOD_FUNC(registry_size), 0);
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_do_get), 1);
    rb_define_method(cRegistry, "build", RUBY_METHOD_FUNC(registry_do_build), -1);
    rb_define_method(cRegistry, "each_type", RUBY_METHOD_FUNC(registry_each_type), 2);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 4);
    rb_define_method(cRegistry, "do_export", RUBY_METHOD_FUNC(registry_export), 2);
    rb_define_method(cRegistry, "merge_xml", RUBY_METHOD_FUNC(registry_merge_xml), 1);
    rb_define_method(cRegistry, "alias", RUBY_METHOD_FUNC(registry_alias), 2);
    rb_define_method(cRegistry, "clear_aliases", RUBY_METHOD_FUNC(registry_clear_aliases), 0);
    rb_define_method(cRegistry, "aliases_of", RUBY_METHOD_FUNC(registry_aliases_of), 1);
    rb_define_method(cRegistry, "merge", RUBY_METHOD_FUNC(registry_merge), 1);
    rb_define_method(cRegistry, "do_minimal", RUBY_METHOD_FUNC(registry_minimal), 2);
    rb_define_method(cRegistry, "includes?", RUBY_METHOD_FUNC(registry_includes_p), 1);
    rb_define_method(cRegistry, "do_resize", RUBY_METHOD_FUNC(registry_resize), 1);
    rb_define_method(cRegistry, "reverse_depends", RUBY_METHOD_FUNC(registry_reverse_depends), 1);
    rb_define_method(cRegistry, "remove", RUBY_METHOD_FUNC(registry_remove), 1);
    rb_define_method(cRegistry, "source_id_of", RUBY_METHOD_FUNC(registry_source_id_of), 1);
    rb_define_method(cRegistry, "do_create_compound", RUBY_METHOD_FUNC(registry_create_compound), 3);
    rb_define_method(cRegistry, "do_create_enum", RUBY_METHOD_FUNC(registry_create_enum), 3);
    rb_define_method(cRegistry, "create_opaque", RUBY_METHOD_FUNC(registry_create_opaque), 2);
    rb_define_method(cRegistry, "create_null", RUBY_METHOD_FUNC(registry_create_null), 1);

    rb_define_singleton_method(cRegistry, "add_standard_cxx_types", RUBY_METHOD_FUNC(registry_add_standard_cxx_types), 1);

    rb_define_singleton_method(cRegistry, "available_containers", RUBY_METHOD_FUNC(registry_available_container), 0);
    rb_define_method(cRegistry, "define_container", RUBY_METHOD_FUNC(registry_define_container), 3);
}

