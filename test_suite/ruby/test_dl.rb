require 'test_config'
require 'typelib'
require 'test/unit'
require '.libs/test_rb_value'
require 'pp'

class TC_DL < Test::Unit::TestCase
    include Typelib
    
    attr_reader :lib, :registry
    def setup
        @registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        @lib = Typelib.dlopen('.libs/test_rb_value.so', registry)
    end

    def test_aliasing
	registry.alias "/my_own_and_only_int", "/int"
	assert_equal(registry.get("my_own_and_only_int"), registry.get("int"))
    end
    def test_wrapping
        wrapper = lib.wrap('test_simple_function_wrapping', "int", "int", "short")
        assert_equal(1, wrapper[1, 2])

        wrapper = lib.wrap('test_simple_function_wrapping', "int", lib.registry.get("int"), "short")
        assert_equal(1, wrapper[1, 2])

        assert_raises(ArgumentError) { lib.wrap('test_simple_function_wrapping', "int", wrapper, "short") }

        wrapper = lib.wrap('test_ptr_passing', "int", "struct A*")
        a = registry.get("struct A").new
        set_struct_A_value(a)
        # Untyped call a.to_dlptr is a pointer on A
        assert_equal(1, wrapper[a.to_dlptr])
        # Now, test struct A => struct A* conversion
        assert_equal(1, wrapper[a])

        wrapper = lib.wrap('test_ptr_return', 'struct A*')
        a = wrapper.call.deference
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)
    end

    def test_ptr_changes
        # Check that parameters passed by pointer are changed
        wrapper = lib.wrap('test_ptr_argument_changes', nil, 'struct B*')
        b = registry.get("struct B").new
        wrapper[b]
        assert(check_B_c_value(b))
    end

    def test_validation
        # Check that nil is not allowed anywhere but at the return value
        assert_raises(ArgumentError) { lib.wrap('test_simple_function_wrapping', "int", nil, "short") }
        assert_nothing_raised { lib.wrap('test_ptr_argument_changes', nil, 'struct B*') }
        GC.start

        # Check that plain structures aren't allowed
        assert_raises(ArgumentError) { lib.wrap('test_simple_function_wrapping', "int", "struct A") }
        # Test output index validation
        assert_raises(ArgumentError) { lib.wrap('test_ptr_argument_changes', [nil, 2], 'struct B*') }
        assert_raises(ArgumentError) { lib.wrap('test_ptr_argument_changes', [nil, -2], 'struct B*') }
        assert_raises(ArgumentError) { lib.wrap('test_ptr_argument_changes', [nil, 0], 'struct B*') }
        assert_raises(ArgumentError) { lib.wrap('test_simple_function_wrapping', ["int", 1], "int") }

        a = registry.get("struct A").new
        b = registry.get("struct B").new
        # Check that pointers are properly typechecked
        wrapper = lib.wrap('test_ptr_return', 'struct A*')
        assert_raises(ArgumentError) { wrapper[b] }
    end

    def test_argument_output
        wrapper = lib.wrap('test_ptr_argument_changes', [nil, 1], 'struct B*')
        b = wrapper.call
        assert(check_B_c_value(b))


        wrapper = lib.wrap('test_arg_input_output', [nil, -1], 'int*', 'INPUT_OUTPUT_MODE')
        assert_raises(ArgumentError) { wrapper[:OUTPUT] }
        assert_equal(5, wrapper[0, :OUTPUT])
        assert_equal(5, wrapper[10, :BOTH])

        wrapper = lib.wrap('test_arg_input_output', [nil, 1], 'int*', 'INPUT_OUTPUT_MODE')
        assert_raises(ArgumentError) { wrapper[10, :BOTH] }
        assert_equal(5, wrapper[:OUTPUT])
        
        wrapper = lib.wrap('test_ptr_argument_changes', [nil, -1], 'struct B*')
        b.a.a = 250;
        new_b = wrapper[b]
        # b and new_b are supposed to be the same objects
        assert_equal(b, new_b)
        assert(check_B_c_value(new_b))

        # Check enum I/O handling
        wrapper = lib.wrap('test_enum_io_handling', [nil, -1], 'INPUT_OUTPUT_MODE*')
        assert_equal(:BOTH, wrapper[:OUTPUT])
        assert_equal(:OUTPUT, wrapper[:BOTH])
    end

    def test_void_return_type
        wrapper = lib.wrap('test_ptr_argument_changes', ["void", 1], 'struct B*')
        b = wrapper.call
        assert(check_B_c_value(b))
    end

    def check_argument_passing(f_def, args = [])
	f_def << 'int'
	check = rand(100)
	args  << check

        wrapper = lib.wrap(*f_def)

	retcheck, *ret = wrapper[*args]
        assert_equal(check, retcheck)
	return ret
    end

    def test_opaque_handling
        wrapper = lib.wrap('test_opaque_handling', 'OpaqueType')
        my_handler = wrapper[]

	check_argument_passing ['check_opaque_value', 'int', 'OpaqueType'], [my_handler]

	id = check_argument_passing ['test_id_handling', ['int', 1], 'DEFINE_ID*']
	check_argument_passing ['check_id_value', 'int', 'DEFINE_ID'], [id]

	wrapper = lib.wrap('test_void_argument', 'int', 'nil*')
	arg = lib.registry.get("int").new
	check = rand(100)
	wrapper[arg]
	assert_equal(check, arg.to_ruby)
    end

    def test_string_handling
        wrapper = lib.wrap('test_string_argument', 'int', 'char*')
        assert_equal(1, wrapper["test"])

        wrapper = lib.wrap('test_string_return', 'char*')
        assert_equal("string_return", wrapper[].to_string)

        wrapper = lib.wrap('test_string_argument_modification', 'void', 'char*', 'int')
        buffer = lib.registry.build("char[256]").new
        wrapper[buffer, 256]
        assert_equal("string_return", buffer.to_string)
    end

end

