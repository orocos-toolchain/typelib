#include <stdio.h>
#include <math.h>
#include <ruby.h>
#include <typelib/value.hh>
#include <test_suite/test_cimport.1>
#include <math.h>

using namespace Typelib;

static bool do_check_struct_A_value(A const& a)
{ 
    if (a.a == 10 && a.b == 20 && a.c == 30 && a.d == 40)
        return true;
    printf("do_check_struct_A_value failed: a=%i, b=%i, c=%i, d=%i\n",
            (int)a.a, (int)a.b, (int)a.c, (int)a.d);
    return false;
}

static void do_set_struct_A_value(A& a)
{
    a.a = 10;
    a.b = 20;
    a.c = 30;
    a.d = 40;
}
/*
 * This file provides the C-side of the test_rb_value testsuite
 */

static VALUE check_struct_A_value(VALUE self, VALUE ra)
{
    Value* value = 0;
    Data_Get_Struct(ra, Value, value);

    A& a(*reinterpret_cast<A*>(value->getData()));
    if (do_check_struct_A_value(a))
        return Qtrue;
    return Qfalse;
}

static VALUE set_struct_A_value(VALUE self, VALUE ra)
{
    Value* value = 0;
    Data_Get_Struct(ra, Value, value);

    A& a(*reinterpret_cast<A*>(value->getData()));
    do_set_struct_A_value(a);
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

static void do_set_B_c_value(B& b)
{
    for (int i = 0; i < 100; ++i)
        b.c[i] = float(i) / 10.0f;
}

static VALUE set_B_c_value(VALUE self, VALUE rb)
{
    Value* value = 0;
    Data_Get_Struct(rb, Value, value);

    B& b(*reinterpret_cast<B*>(value->getData()));
    do_set_B_c_value(b);
    return Qnil;
}

extern "C" void Init_test_rb_value()
{
    rb_define_method(rb_mKernel, "check_B_c_value",         RUBY_METHOD_FUNC(check_B_c_value), 1);
    rb_define_method(rb_mKernel, "set_B_c_value",           RUBY_METHOD_FUNC(set_B_c_value), 1);
    rb_define_method(rb_mKernel, "check_struct_A_value",    RUBY_METHOD_FUNC(check_struct_A_value), 1);
    rb_define_method(rb_mKernel, "set_struct_A_value",      RUBY_METHOD_FUNC(set_struct_A_value), 1);
}

extern "C" void init_dl_wrapping() { }
/* Testing function wrapped by Typelib::wrap (using Ruby::DL)
 * The function is supposed to return 1 if the arguments have these values:
 *   first  == 1
 *   second == 0.02
 */
extern "C" int test_simple_function_wrapping(int first, short second)
{
    if (first == 1 && second == 2)
        return 1;
    printf("test_simple_function_wrapping failed: first=%i second=%i\n", first, second);
    return 0;
}

extern "C" int test_ptr_passing(A* a)
{
    if (do_check_struct_A_value(*a))
        return 1;
    return 0;
}

extern "C" struct A* test_ptr_return()
{
    static A a;
    do_set_struct_A_value(a);
    return &a;
}

extern "C" void test_ptr_argument_changes(struct B* b)
{ do_set_B_c_value(*b); }

extern "C" void test_arg_input_output(int* value, INPUT_OUTPUT_MODE mode) 
{ 
    if (mode == BOTH && *value != 10)
    {
        *value = 0;
        return;
    }
    *value = 5; 
}

extern "C" void test_enum_io_handling(INPUT_OUTPUT_MODE* mode)
{
    switch(*mode)
    {
        case BOTH:
            *mode = OUTPUT;
            break;
        case OUTPUT:
            *mode = BOTH;
            break;
    }
}

