#include <ruby.h>
extern "C" {
#include <st.h>
}

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


static void
memory_unref(void *ptr)
{
    st_delete(MemoryTable, (st_data_t*)&ptr, 0);
}

static void
memory_delete(void *ptr)
{
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

void Typelib_init_memory()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    MemoryTable = st_init_table(&memory_table_type);

    cMemoryZone = rb_define_class_under(mTypelib, "MemoryZone", rb_cObject);
    rb_define_method(cMemoryZone, "zone_address", RUBY_METHOD_FUNC(memory_zone_address), 0);
    rb_define_method(cMemoryZone, "to_ptr", RUBY_METHOD_FUNC(memory_zone_to_ptr), 0);

    rb_define_method(rb_cString, "to_memory_ptr", RUBY_METHOD_FUNC(string_to_memory_ptr), 0);
}

