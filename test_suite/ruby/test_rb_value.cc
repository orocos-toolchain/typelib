#include <ruby.h>
#include <typelib/value.hh>
#include <test_suite/test_cimport.1>
#include <math.h>

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

static VALUE check_B_c_value(VALUE self, VALUE rb)
{
    Value* value = 0;
    Data_Get_Struct(rb, Value, value);

    B& b(*reinterpret_cast<B*>(value->getData()));
    for (int i = 0; i < 100; ++i)
        if (fabs(b.c[i] - float(i) / 10.0f) > 0.001)
            return Qfalse;
    return Qtrue;
}

static VALUE set_B_c_value(VALUE self, VALUE rb)
{
    Value* value = 0;
    Data_Get_Struct(rb, Value, value);

    B& b(*reinterpret_cast<B*>(value->getData()));
    for (int i = 0; i < 100; ++i)
        b.c[i] = float(i) / 10.0f;
    return Qnil;
}

extern "C" void Init_test_rb_value()
{
    rb_define_method(rb_mKernel, "get_B_c_value", RUBY_METHOD_FUNC(check_B_c_value), 1);
    rb_define_method(rb_mKernel, "set_B_c_value", RUBY_METHOD_FUNC(set_B_c_value), 1);
    rb_define_method(rb_mKernel, "check_struct_A_value", RUBY_METHOD_FUNC(check_struct_A_value), 1);
    rb_define_method(rb_mKernel, "set_struct_A_value", RUBY_METHOD_FUNC(set_struct_A_value), 1);
}

