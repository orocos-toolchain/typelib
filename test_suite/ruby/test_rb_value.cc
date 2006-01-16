#include <ruby.h>
#include <typelib/value.hh>
#include <test_suite/test_cimport.1>

using namespace Typelib;

/*
 * This file provides the C-side of the test_rb_value testsuite
 */

static VALUE check_struct_A_value(VALUE self, VALUE ra)
{
    Value* value = 0;
    Data_Get_Struct(ra, Value, value);

    A& a(*reinterpret_cast<A*>(value->getData()));
    if (a.a == 1 && a.b == 2 && a.c == 3 && a.d == 4)
        return Qtrue;
    return Qfalse;
}

static VALUE set_struct_A_value(VALUE self, VALUE ra)
{
    Value* value = 0;
    Data_Get_Struct(ra, Value, value);

    A& a(*reinterpret_cast<A*>(value->getData()));
    a.a = 10;
    a.b = 20;
    a.c = 30;
    a.d = 40;
    return ra;
}

extern "C" void Init_test_rb_value()
{
    rb_define_method(rb_mKernel, "check_struct_A_value", RUBY_METHOD_FUNC(check_struct_A_value), 1);
    rb_define_method(rb_mKernel, "set_struct_A_value", RUBY_METHOD_FUNC(set_struct_A_value), 1);
}

