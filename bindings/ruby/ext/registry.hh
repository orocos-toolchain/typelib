/***********************************************************************************
 *
 * Wrapping of the Registry class
 *
 */


static 
void registry_free(void* ptr) { delete reinterpret_cast<Registry*>(ptr); }

static
VALUE registry_alloc(VALUE klass)
{
    Registry* registry = new Registry;
    return Data_Wrap_Struct(klass, 0, registry_free, registry);
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

/* Private method to import a given file in the registry
 * We expect Registry#import to format the arguments before calling
 * do_import
 */
static
VALUE registry_import(VALUE self, VALUE file, VALUE kind, VALUE options)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    
    PluginManager::self manager;
    Importer* importer = manager->importer( StringValuePtr(kind) );

    config_set config;
    if (! NIL_P(options))
    {
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
            else
                config.set(StringValuePtr(k), StringValuePtr(v));
        }
    }
        
    // TODO: error checking
    if (!importer->load(StringValuePtr(file), config, registry))
        rb_raise(rb_eRuntimeError, "cannot import %s", StringValuePtr(file));

    return Qnil;
}


