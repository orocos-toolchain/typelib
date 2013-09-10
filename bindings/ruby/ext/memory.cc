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

static VALUE cMemoryZone;
struct MemoryZone
{
    void* ptr;
    MemoryZone(void* ptr)
        : ptr(ptr) {}
};
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

struct MemoryTableEntry
{
    int refcount;
    VALUE object;
    bool owned;
    void* root_ptr;
    MemoryTableEntry(VALUE object, bool owned, void* root_ptr = 0)
        : refcount(1), object(object), owned(owned), root_ptr(root_ptr)
    {
        if (root_ptr && owned)
            rb_raise(rb_eArgError, "given both a root pointer and owned=true for object %llu", NUM2ULL(rb_obj_id(object)));
    }
};

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

#ifdef VERBOSE
static int
memory_touch_i(volatile void* ptr, MemoryTableEntry* entry, st_data_t)
{
    char value = *((char*)ptr);
    fprintf(stderr, "touching %p (refcount=%i)\n", ptr, entry->refcount);
    *((char*)ptr) = value;
    return ST_CONTINUE;
}

static void
memory_touch_all()
{
    st_foreach(MemoryTable, (int(*)(ANYARGS))memory_touch_i, (st_data_t)0);

}
#endif

bool
typelib_ruby::memory_ref(void *ptr)
{
#   ifdef VERBOSE
    fprintf(stderr, "%p: ref\n", ptr);
#   endif

    MemoryTableEntry* entry = 0;
    if (st_lookup(MemoryTable, (st_data_t)ptr, (st_data_t*)&entry))
    {
        ++(entry->refcount);
        return true;
    }
    else
    {
        return false;
    }
}

static void
memory_zone_unref(MemoryZone* ptr)
{
    // ptr->ptr is NULL if the zone has been invalidated
    if (ptr->ptr)
        memory_unref(ptr->ptr);
}

void
typelib_ruby::memory_unref(void *ptr)
{
#   ifdef VERBOSE
    fprintf(stderr, "%p: unref\n", ptr);
#   endif

    MemoryTableEntry* entry = 0;
    if (!st_lookup(MemoryTable, (st_data_t)ptr, (st_data_t*)&entry))
        rb_raise(rb_eArgError, "cannot find %p in memory table", ptr);
    --(entry->refcount);
    if (!entry->refcount)
    {
        if (entry->owned)
            memory_delete(ptr);
        if (entry->root_ptr)
            memory_unref(entry->root_ptr);

#       ifdef VERBOSE
        fprintf(stderr, "%p: deregister\n", ptr);
#       endif
        delete entry;
        st_delete(MemoryTable, (st_data_t*)&ptr, 0);
    }

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

void
typelib_ruby::memory_delete(void *ptr)
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

#   ifdef VERBOSE
    fprintf(stderr, "%p: deallocated\n", ptr);
#   endif
    ruby_xfree(ptr);
}

static VALUE
memory_aref(void *ptr)
{
    MemoryTableEntry* entry;
    if (!st_lookup(MemoryTable, (st_data_t)ptr, (st_data_t*)&entry)) {
	return Qnil;
    }
    if ((VALUE)entry == Qundef)
	rb_bug("found undef in memory table");

    return entry->object;
}

static void
memory_aset(void *ptr, VALUE obj, bool owned, void* root_ptr)
{
    if (! NIL_P(memory_aref(ptr)))
	rb_raise(rb_eArgError, "there is already a wrapper for %p", ptr);

    if (ptr == root_ptr)
	rb_raise(rb_eArgError, "pointer and root pointer are equal");

    if (!memory_ref(ptr))
    {
        MemoryTableEntry* entry = new MemoryTableEntry(obj, owned, root_ptr);
        st_insert(MemoryTable, (st_data_t)ptr, (st_data_t)entry);

        if (root_ptr)
        {
            MemoryTableEntry* root_entry = 0;
            if (!st_lookup(MemoryTable, (st_data_t)root_ptr, (st_data_t*)&root_entry))
                rb_raise(rb_eArgError, "%p given as root pointer for %p but is not registered", root_ptr, ptr);
            ++(root_entry->refcount);
        }
    }
}

VALUE
typelib_ruby::memory_allocate(size_t size)
{
#   ifdef VERBOSE
    memory_touch_all();
#   endif

    void* ptr = ruby_xmalloc(size);
#   ifdef VERBOSE
    fprintf(stderr, "%p: new allocated zone of size %lu\n", ptr, size);
#   endif

    return memory_wrap(ptr, true, 0);
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
typelib_ruby::memory_wrap(void* ptr, bool take_ownership, void* root_ptr)
{
    VALUE zone = memory_aref(ptr);
    if (NIL_P(zone))
    {
#	ifdef VERBOSE
	fprintf(stderr, "%p: wrapping new memory zone\n", ptr);
#	endif

        zone = Data_Wrap_Struct(cMemoryZone, 0, &memory_zone_unref, new MemoryZone(ptr));
	memory_aset(ptr, zone, take_ownership, root_ptr);
    }
    else
    {
#	ifdef VERBOSE
	fprintf(stderr, "%p: already known memory zone\n", ptr);
#	endif
    }

    return zone;
}

void*
typelib_ruby::memory_cptr(VALUE ptr)
{
    MemoryZone* cptr;
    Data_Get_Struct(ptr, MemoryZone, cptr);
    return cptr->ptr;
}

static MemoryZone*
memory_zone(VALUE ptr)
{
    MemoryZone* cptr;
    Data_Get_Struct(ptr, MemoryZone, cptr);
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
#   ifdef VERBOSE
    fprintf(stderr, "allocating void* to create a pointer-to-memory\n");
#   endif

    VALUE result = memory_allocate(sizeof(void*));

    void* newptr = memory_cptr(result);
    void* ptr    = memory_cptr(self);

    *reinterpret_cast<void**>(newptr) = ptr;
    rb_iv_set(result, "@pointed_to_memory", self);
    return result;
}

static VALUE
memory_zone_invalidate(VALUE self)
{
    MemoryZone* cptr = memory_zone(self);
#   ifdef VERBOSE
    fprintf(stderr, "invalidating memory zone %p", cptr->ptr);
#   endif
    if (cptr->ptr)
        memory_unref(cptr->ptr);
    cptr->ptr = 0;
    return Qnil;
}

static VALUE
string_to_memory_ptr(VALUE self)
{
    rb_str_modify(self);
#   ifdef VERBOSE
    fprintf(stderr, "wrapping string value from Ruby\n");
#   endif
    VALUE ptr = memory_wrap(StringValuePtr(self), false, 0);
    rb_iv_set(ptr, "@buffer_string", self);
    return ptr;
}

static VALUE
memory_zone_table_size(VALUE self)
{
    return INT2NUM(MemoryTable->num_entries);
}

void typelib_ruby::Typelib_init_memory()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    MemoryTable     = st_init_table(&memory_table_type);
    VALUE table     = Data_Wrap_Struct(rb_cObject, 0, 0, MemoryTable);
    rb_iv_set(mTypelib, "@__memory_table__", table);

    cMemoryZone = rb_define_class_under(mTypelib, "MemoryZone", rb_cObject);
    rb_define_method(cMemoryZone, "zone_address", RUBY_METHOD_FUNC(memory_zone_address), 0);
    rb_define_method(cMemoryZone, "to_ptr", RUBY_METHOD_FUNC(memory_zone_to_ptr), 0);
    rb_define_method(cMemoryZone, "invalidate", RUBY_METHOD_FUNC(memory_zone_invalidate), 0);
    rb_define_singleton_method(cMemoryZone, "table_size", RUBY_METHOD_FUNC(memory_zone_table_size), 0);

    rb_define_method(rb_cString, "to_memory_ptr", RUBY_METHOD_FUNC(string_to_memory_ptr), 0);
}

