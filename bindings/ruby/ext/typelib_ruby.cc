#include "typelib.hh"

#include <typelib/typevisitor.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <utilmm/configfile/configset.hh>

using namespace Typelib;
using utilmm::config_set;
using std::string;

using namespace typelib_ruby;
static VALUE mTypelib   = Qnil;

/**********************************************************************
 *
 * Extension of the standard libraries
 *
 */

static VALUE kernel_is_immediate(VALUE klass, VALUE object)
{ return IMMEDIATE_P(object) ? Qtrue : Qfalse; }
/* call-seq:
 *  numeric?	=> true or false
 *
 * Returns true if the receiver is either a Fixnum or a Float
 */
static VALUE kernel_is_numeric(VALUE klass, VALUE object)
{ 
    return (FIXNUM_P(object) || TYPE(object) == T_FLOAT || TYPE(object) == T_BIGNUM) ? Qtrue : Qfalse;
}


static VALUE typelib_with_dyncall(VALUE klass)
{
    return
#ifdef WITH_DYNCALL
        Qtrue;
#else
        Qfalse;
#endif
}

extern "C" void Init_typelib_ruby()
{
    mTypelib  = rb_define_module("Typelib");
#ifdef WITH_DYNCALL
    Typelib_init_functions();
#endif
    Typelib_init_values();
    Typelib_init_strings();
    Typelib_init_registry();
    Typelib_init_memory();
    Typelib_init_metadata();
    
    rb_define_singleton_method(mTypelib, "with_dyncall?", RUBY_METHOD_FUNC(typelib_with_dyncall), 0);
    rb_define_singleton_method(rb_mKernel, "immediate?", RUBY_METHOD_FUNC(kernel_is_immediate), 1);
    rb_define_singleton_method(rb_mKernel, "numeric?", RUBY_METHOD_FUNC(kernel_is_numeric), 1);
}

