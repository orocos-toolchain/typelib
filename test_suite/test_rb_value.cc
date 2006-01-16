#include <ruby.h>
#include <typelib/value.hh>
#include "test_cimport.1"

/*
 * This file provides the C-side of the test_rb_value testsuite
 */

static VALUE check_struct_A_value(VALUE self, VALUE a)
{
    Value* value = 0;
    Data_Get_Struct(a, Value, value);

    A& a(value_cast<A>(*value));
    if (a.a == 1 && a.b == 2 && a.c == 3 && a.d == 4)
        return Qtrue;
    return Qfalse;
}

static VALUE set_struct_A_value(VALUE self, VALUE a)
{
    Value* value = 0;
    Data_Get_Struct(a, Value, value);

    A& a(value_cast<A>(*value));
    a.a = 10;
    a.b = 20;
    a.c = 30;
    a.d = 40;
    return a;
}

void Init_test_rb_value()
{
    rb_define_method(rb_mKernel, "check_struct_A_value", RUBY_METHOD_FUNC(check_struct_A_value), 1);
    rb_define_method(rb_mKernel, "set_struct_A_value", RUBY_METHOD_FUNC(set_struct_A_value), 1);
}

