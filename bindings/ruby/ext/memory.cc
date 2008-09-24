#include "typelib.hh"
#include <typelib/value_ops.hh>
#include <ruby.h>
extern "C" {
#include <st.h>
}

using namespace Typelib;
using namespace std;
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
static int memory_table_hash(void* a)
{
    /* Use the low-order bits as hash value, as they are the most likely to
     * change */
    return (long)a;
}

static struct st_hash_type memory_table_type = {
    (int (*)())memory_table_compare,
    (int (*)())memory_table_hash
};

typedef std::map<void*, VALUE> MemoryTypes;
MemoryTypes memory_types;


static void
memory_unref(void *ptr)
{
    st_delete(MemoryTable, (st_data_t*)&ptr, 0);
    memory_types.erase(ptr);
}

static void
memory_delete(void *ptr)
{
    MemoryTypes::iterator it = memory_types.find(ptr);
    if (it != memory_types.end())
    {
        Type const& type = rb2cxx::object<Type>(it->second);
        Typelib::destroy(Value(ptr, type));
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
	rb_raise(rb_eArgError, "there is already a wrapper for %x", ptr);

    st_insert(MemoryTable, (st_data_t)ptr, obj);
}

VALUE
memory_allocate(size_t size)
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
memory_init(VALUE ptr, VALUE type)
{
    void* cptr = memory_cptr(ptr);
    MemoryTypes::iterator it = memory_types.find(cptr);
    if (it != memory_types.end())
        rb_raise(rb_eArgError, "memory zone already initialized");

    // For deinitialization later
    Type const& t(rb2cxx::object<Type>(type));
    memory_types.insert( make_pair(cptr, type) );
    Typelib::init(Value(cptr, t));
}

VALUE
memory_wrap(void* ptr)
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
memory_cptr(VALUE ptr)
{
    void* cptr;
    Data_Get_Struct(ptr, void, cptr);
    return cptr;
}

static VALUE
memory_zone_address(VALUE self)
{
    void* ptr = memory_cptr(self);
    return LONG2NUM((long)ptr);
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
    for (MemoryTypes::iterator it = memory_types.begin(); it != memory_types.end(); ++it)
        rb_gc_mark(it->second);
}

void Typelib_init_memory()
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

