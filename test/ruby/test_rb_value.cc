#include <stdio.h>
#include <math.h>
#include <ruby.h>
#include <typelib/value.hh>
#include <test/test_cimport.1>
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

static VALUE fill_multi_dim_array(VALUE self, VALUE rb)
{
    Value* value = 0;
    Data_Get_Struct(rb, Value, value);

    TestMultiDimArray& b(*reinterpret_cast<TestMultiDimArray*>(value->getData()));
    for (int y = 0; y < 10; ++y)
	for (int x = 0; x < 10; ++x)
	    b.fields[y][x] = x + y * 10;

    return Qnil;
}

extern "C" {

    void Init_libtest_ruby()
    {
        rb_define_method(rb_mKernel, "check_B_c_value",         RUBY_METHOD_FUNC(check_B_c_value), 1);
        rb_define_method(rb_mKernel, "set_B_c_value",           RUBY_METHOD_FUNC(set_B_c_value), 1);
        rb_define_method(rb_mKernel, "check_struct_A_value",    RUBY_METHOD_FUNC(check_struct_A_value), 1);
        rb_define_method(rb_mKernel, "set_struct_A_value",      RUBY_METHOD_FUNC(set_struct_A_value), 1);
        rb_define_method(rb_mKernel, "fill_multi_dim_array",      RUBY_METHOD_FUNC(fill_multi_dim_array), 1);
    }

    void test_simple_function_call() { }

    void test_numeric_argument_passing(char a, short b, int c, long d, long long e, float f, double g)
    {
	if (a != 1 || b != 2 || c != 3 || d != 4 || e != 5 || f != 6 || g != 7)
	    rb_raise(rb_eArgError, "failed to pass numeric arguments");
    }
    char test_returns_numeric_argument_char(char value) { return value; }
    short test_returns_numeric_argument_short(short value) { return value; }
    int test_returns_numeric_argument_int(int value) { return value; }
    long test_returns_numeric_argument_long(long value) { return value; }
    int64_t test_returns_numeric_argument_int64_t(int64_t value) { return value; }
    float test_returns_numeric_argument_float(float value) { return value; }
    double test_returns_numeric_argument_double(double value) { return value; }
    void test_returns_argument(int* holder) { *holder = 42; }


    void test_pointer_argument(A* a)
    {
        if (!do_check_struct_A_value(*a))
            rb_raise(rb_eArgError, "error in passing structure as pointer");
    }

    struct A* test_returns_pointer()
    {
        static A a;
        do_set_struct_A_value(a);
        return &a;
    }

    void test_modifies_argument(int* value) 
    { 
	if (*value != 0)
	    rb_raise(rb_eArgError, "invalid argument value");
	*value = 42; 
    }


    int test_immediate_to_pointer(double* value) { return *value == 0.5; }

    void test_ptr_argument_changes(struct B* b)
    { do_set_B_c_value(*b); }

    void test_arg_input_output(int* value, INPUT_OUTPUT_MODE mode) 
    { 
        if (mode == BOTH && *value != 10)
        {
            *value = 0;
            return;
        }
        *value = 5; 
    }

    void test_enum_io_handling(INPUT_OUTPUT_MODE* mode)
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

    static int opaque_handler;
    OPAQUE_TYPE test_opaque_handling()
    { return &opaque_handler; }

    int check_opaque_value(OPAQUE_TYPE handler, int check)
    { return (handler == test_opaque_handling()) ? check : 0; }

    void test_string_argument(char const* value)
    {
        if (strcmp(value, "test"))
            rb_raise(rb_eArgError, "bad string argument");
    }

    static const char* static_string = "string_return";
    const char* test_string_return()
    { return static_string; }
    void test_string_argument_modification(char* str, int buffer_length)
    { strcpy(str, static_string); }
    void test_string_as_array(char str[256])
    { strcpy(str, static_string); }

    DEFINE_STR id;
    int test_id_handling(DEFINE_ID* new_id, int check)
    {
        *new_id = &id;
        return check;
    }
    int check_id_value(DEFINE_ID test_id, int check)
    { 
        printf("check_id_value, test_id=%p, &id=%p\n", test_id, &id);
        return (test_id == &id) ? check : 0; 
    }

    void test_null_return_value(DEFINE_ID* test_id, int check)
    {
	*test_id = 0;
    }

    void test_void_argument(void* value, int check)
    { *static_cast<int*>(value) = check; }
}


