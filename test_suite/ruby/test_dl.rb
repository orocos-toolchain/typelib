require 'test_config'
require 'typelib'
require 'test/unit'
require '.libs/test_rb_value'
require 'pp'

class TC_DL < Test::Unit::TestCase
    include Typelib
    
    attr_reader :test_lib, :registry
    def setup
        @registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        @test_lib = Typelib.dlopen('.libs/test_rb_value.so', registry)
    end

    def test_wrapping
        typelib_wrap = test_lib.wrap('test_simple_function_wrapping', "int", "int", "short")
        assert_equal(1, typelib_wrap[1, 2])

        typelib_wrap = test_lib.wrap('test_ptr_passing', "int", "struct A*")
        a = Value.new(nil, registry.get("struct A"))
        set_struct_A_value(a)
        assert_equal(1, typelib_wrap[a.to_ptr])

        # Now, test simple type coercion
        assert_equal(1, typelib_wrap[a])

        typelib_wrap = test_lib.wrap('test_ptr_return', 'struct A*')
        a = typelib_wrap.call
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)

    end

    def test_ptr_changes
        # Check that parameters passed by pointer are changed
        typelib_wrap = test_lib.wrap('test_ptr_argument_changes', nil, 'struct B*')
        b = Value.new(nil, registry.get("struct B"))
        typelib_wrap[b]
        check_B_c_value(b)
    end

    def test_validation
        # Check that nil is not allowed anywhere but at the return value
        assert_raises(ArgumentError) { test_lib.wrap('test_simple_function_wrapping', "int", nil, "short") }
        assert_nothing_raised { test_lib.wrap('test_ptr_argument_changes', nil, 'struct B*') }
        GC.start

        # Check that plain structures aren't allowed
        assert_raises(ArgumentError) { test_lib.wrap('test_simple_function_wrapping', "int", "struct A") }

        a = Value.new(nil, registry.get("struct A"))
        b = Value.new(nil, registry.get("struct B"))
        # Check that pointers are properly typechecked
        typelib_wrap = test_lib.wrap('test_ptr_return', 'struct A*')
        assert_raises(ArgumentError) { typelib_wrap[b] }
    end

    def test_argument_output
        typelib_wrap = test_lib.wrap('test_ptr_argument_changes', [nil, 1], 'struct B*')
        b = typelib_wrap.call
        check_B_c_value(b)

        typelib_wrap = test_lib.wrap('test_arg_input_output', [nil, -1], 'int*', 'INPUT_OUTPUT_MODE')
        assert_raises(ArgumentError) { typelib_wrap[:OUTPUT] }
        assert_equal(5, typelib_wrap[0, :OUTPUT])
        assert_equal(5, typelib_wrap[10, :BOTH])

        typelib_wrap = test_lib.wrap('test_arg_input_output', [nil, 1], 'int*', 'INPUT_OUTPUT_MODE')
        assert_raises(ArgumentError) { typelib_wrap[10, :BOTH] }
        assert_equal(5, typelib_wrap[:OUTPUT])
        
        typelib_wrap = test_lib.wrap('test_ptr_argument_changes', [nil, -1], 'struct B*')
        b.a = 250;
        new_b = typelib_wrap[b]
        # b and new_b are supposed to be the same objects
        assert_equal(b, new_b)
        check_B_c_value(new_b)

        # Check enum I/O handling
        typelib_wrap = test_lib.wrap('test_enum_io_handling', [nil, -1], 'INPUT_OUTPUT_MODE*')
        assert_equal(:BOTH, typelib_wrap[:OUTPUT])
        assert_equal(:OUTPUT, typelib_wrap[:BOTH])
    end

    def test_void_return_type
        typelib_wrap = test_lib.wrap('test_ptr_argument_changes', ["void", 1], 'struct B*')
        b = typelib_wrap.call
        check_B_c_value(b)
    end

    def test_opaque_handling
        typelib_wrap = test_lib.wrap('test_opaque_handling', "OpaqueType")
        my_handler = typelib_wrap[]
        typelib_wrap = test_lib.wrap('check_opaque_value', 'int', 'OpaqueType')
        assert_equal(1, typelib_wrap[my_handler])
    end
end

