#include "typelib.hh"

#include <typelib/typevisitor.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <utilmm/configfile/configset.hh>

using namespace Typelib;
using utilmm::config_set;
using std::string;

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
    return (FIXNUM_P(object) || TYPE(object) == T_FLOAT) ? Qtrue : Qfalse;
}

extern "C" void Init_typelib_ruby()
{
    mTypelib  = rb_define_module("Typelib");
    Typelib_init_functions();
    Typelib_init_values();
    Typelib_init_strings();
    Typelib_init_registry();
    Typelib_init_memory();
    
    rb_define_singleton_method(rb_mKernel, "immediate?", RUBY_METHOD_FUNC(kernel_is_immediate), 1);
    rb_define_singleton_method(rb_mKernel, "numeric?", RUBY_METHOD_FUNC(kernel_is_numeric), 1);
}

