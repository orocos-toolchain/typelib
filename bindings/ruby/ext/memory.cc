#include "typelib.hh"
#include <typelib/value_ops.hh>
#include <ruby.h>
#if !defined(RUBY_19) && !defined(RUBY_191)
extern "C" {
#include <st.h>
}
#endif

using namespace Typelib;
using namespace std;
using namespace typelib_ruby;
#undef VERBOSE

static VALUE cMemoryZone;
static st_table* MemoryTable;

/* For those who are wondering, st_data_t is always the size of an void*
 * (see the first lines of st.h
 */
static int memory_table_compare(void* a, void* b)
{
    return (a != b);
}

#if !defined(RUBY_19) && !defined(RUBY_191)
typedef long st_index_t;
#endif

static st_index_t memory_table_hash(void* a)
{
    /* Use the low-order bits as hash value, as they are the most likely to
     * change */
    return (st_index_t)a;
}

#if defined(RUBY_19)
static struct st_hash_type memory_table_type = {
    (int (*)(...))memory_table_compare,
    (st_index_t (*)(...))memory_table_hash
};
#elif defined(RUBY_191)
static struct st_hash_type memory_table_type = {
    (int (*)(...))memory_table_compare,
    (int (*)(...))memory_table_hash
};
#else
static struct st_hash_type memory_table_type = {
    (int (*)())memory_table_compare,
    (int (*)())memory_table_hash
};
#endif

struct RbMemoryLayout
{
    int refcount;
    MemoryLayout layout;
    boost::shared_ptr<Registry> registry;

    RbMemoryLayout()
        : refcount(0) {}
    RbMemoryLayout(MemoryLayout const& layout, boost::shared_ptr<Registry> registry)
        : refcount(0), layout(layout), registry(registry) {}
};

// MemoryTypes actually holds the memory layout of each memory zone. We cannot
// simply use the Type object as, at exit, the Ruby GC do not order finalization
// and therefore the registry can be deleted before the Type instances.
typedef std::map< void const*, void const* > MemoryTypes;
typedef std::map< void const*, RbMemoryLayout > TypeLayouts;
MemoryTypes memory_types;
TypeLayouts memory_layouts;

static void
memory_unref(void *ptr)
{
    st_delete(MemoryTable, (st_data_t*)&ptr, 0);

    MemoryTypes::iterator type_it = memory_types.find(ptr);
    if (type_it != memory_types.end())
    {
        TypeLayouts::iterator layout_it = memory_layouts.find(type_it->second);
        RbMemoryLayout& layout = layout_it->second;
        if (0 == --layout.refcount)
            memory_layouts.erase(layout_it);
        memory_types.erase(type_it);
    }
}

static void
memory_delete(void *ptr)
{
    MemoryTypes::iterator it = memory_types.find(ptr);
    if (it != memory_types.end())
    {
        TypeLayouts::iterator layout_it = memory_layouts.find(it->second);
        if (layout_it != memory_layouts.end())
        {
            RbMemoryLayout& layout = layout_it->second;
            Typelib::destroy(
                    static_cast<uint8_t*>(ptr),
                    layout.layout);
        }
    }

    free(ptr);
    memory_unref(ptr);
}

static VALUE
memory_aref(void *ptr)
{
    VALUE val;

    if (!st_lookup(MemoryTable, (st_data_t)ptr, &val)) {
	return Qnil;
    }
    if (val == Qundef)
	rb_bug("found undef in memory table");

    return val;
}

static void
memory_aset(void *ptr, VALUE obj)
{
    if (! NIL_P(memory_aref(ptr)))
	rb_raise(rb_eArgError, "there is already a wrapper for %p", ptr);

    st_insert(MemoryTable, (st_data_t)ptr, obj);
}

VALUE
typelib_ruby::memory_allocate(size_t size)
{
    void* ptr = malloc(size);
    VALUE zone = Data_Wrap_Struct(cMemoryZone, 0, &memory_delete, ptr);
#   ifdef VERBOSE
    fprintf(stderr, "%x: new allocated zone of size %i\n", ptr, size);
#   endif
    memory_aset(ptr, zone);
    return zone;
}

void
typelib_ruby::memory_init(VALUE ptr, VALUE type)
{
    try {
        void* cptr = memory_cptr(ptr);
        MemoryTypes::iterator it = memory_types.find(cptr);
        if (it != memory_types.end())
            rb_raise(rb_eArgError, "memory zone already initialized");

        // For deinitialization later, get or register the type's layout
        Type const& t(rb2cxx::object<Type>(type));
        TypeLayouts::iterator layout_it = memory_layouts.find(&t);
        if (layout_it == memory_layouts.end())
        {
            cxx2rb::RbRegistry& registry = rb2cxx::object<cxx2rb::RbRegistry>(type_get_registry(type));
            layout_it = memory_layouts.insert(
                make_pair( &t, RbMemoryLayout(layout_of(t, true), registry.registry) )
                ).first;
        }
        RbMemoryLayout& layout = layout_it->second;
        ++layout.refcount;

        memory_types.insert( make_pair(cptr, &t) );
        Typelib::init(static_cast<uint8_t*>(cptr), layout.layout);
    } catch(std::exception const& e) {
        rb_raise(rb_eArgError, "internal error: %s", e.what());
    }
}

VALUE
typelib_ruby::memory_wrap(void* ptr)
{
    VALUE zone = memory_aref(ptr);
    if (NIL_P(zone))
    {
#	ifdef VERBOSE
	fprintf(stderr, "%x: wrapping new memory zone\n", ptr);
#	endif

	zone = Data_Wrap_Struct(cMemoryZone, 0, &memory_unref, ptr);
	memory_aset(ptr, zone);
    }
    else
    {
#	ifdef VERBOSE
	fprintf(stderr, "%x: already known memory zone\n", ptr);
#	endif
    }

    return zone;
}

void*
typelib_ruby::memory_cptr(VALUE ptr)
{
    void* cptr;
    Data_Get_Struct(ptr, void, cptr);
    return cptr;
}

static VALUE
memory_zone_address(VALUE self)
{
    void* ptr = memory_cptr(self);
    return ULL2NUM(reinterpret_cast<uint64_t>(ptr));
}

static VALUE
memory_zone_to_ptr(VALUE self)
{
    VALUE result = memory_allocate(sizeof(void*));

    void* newptr = memory_cptr(result);
    void* ptr    = memory_cptr(self);

    *reinterpret_cast<void**>(newptr) = ptr;
    rb_iv_set(result, "@pointed_to_memory", self);
    return result;
}

static VALUE
string_to_memory_ptr(VALUE self)
{
    rb_str_modify(self);
    VALUE ptr = memory_wrap(StringValuePtr(self));
    rb_iv_set(ptr, "@buffer_string", self);
    return ptr;
}

void memory_table_mark(void* ptr)
{
}

void typelib_ruby::Typelib_init_memory()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    MemoryTable     = st_init_table(&memory_table_type);
    VALUE table     = Data_Wrap_Struct(rb_cObject, &memory_table_mark, 0, MemoryTable);
    rb_iv_set(mTypelib, "@__memory_table__", table);

    cMemoryZone = rb_define_class_under(mTypelib, "MemoryZone", rb_cObject);
    rb_define_method(cMemoryZone, "zone_address", RUBY_METHOD_FUNC(memory_zone_address), 0);
    rb_define_method(cMemoryZone, "to_ptr", RUBY_METHOD_FUNC(memory_zone_to_ptr), 0);

    rb_define_method(rb_cString, "to_memory_ptr", RUBY_METHOD_FUNC(string_to_memory_ptr), 0);
}

