require 'typelib/test'

if Typelib.with_dyncall?
class TC_Functions < Minitest::Test
    include Typelib
    
    attr_reader :lib, :registry
    def setup
        @registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        @lib = Library.open(File.join(BUILDDIR, 'ruby', 'libtest_ruby.so'), registry)
    end

    def test_opening
	assert_raises(ArgumentError) { Library.open('does_not_exist') }
    end

    def test_simple_function_call
        wrapper = lib.find('test_simple_function_call')
	assert_equal(nil, wrapper.call);
    end

    def test_void_return_type
        wrapper = lib.find('test_ptr_argument_changes').
	    returns("void").returns('B*')
        b = wrapper.call
        assert(check_B_c_value(b))
    end

    NUMERIC_TYPES = %w{char short int long int64_t float double}
    def test_numeric_argument_passing
        wrapper = lib.find('test_numeric_argument_passing').
	    with_arguments(*NUMERIC_TYPES)

	assert_equal(NUMERIC_TYPES.size, wrapper.arity)
	assert_equal(nil, wrapper.call(*(1..NUMERIC_TYPES.size).to_a));
    end

    def test_returns_numeric_argument
	NUMERIC_TYPES.each do |name|
	    wrapper = lib.find("test_returns_numeric_argument_#{name}").
		returns(name).
		with_arguments(name)

	    assert( wrapper.returns_something? )
	    assert_equal(lib.registry.get("/#{name}").name, wrapper.return_type.name)
	    assert_equal(1,  wrapper.arity)

	    assert_equal(12, wrapper.call(12));
	end
    end

    def test_returns_null_pointer
        wrapper = lib.find('test_null_return_value').
	    returns(nil).returns('DEFINE_ID*')
        assert_equal(nil, wrapper.call)
    end

    def test_pointer_argument
	wrapper = lib.find('test_pointer_argument').
	    with_arguments("A*")

	a = registry.get("A").new
	a.a = 10
	a.b = 20
	a.c = 30
	a.d = 40

	# Untyped call a.to_memory_ptr is a pointer on A
	wrapper[a.to_memory_ptr]
	# Now, test struct A => struct A* conversion
	wrapper[a]

        # Check that pointers are properly typechecked
        b = registry.get("B").new
        assert_raises(TypeError) { wrapper[b] }
    end

    def check_B_c_value(b)
	100.times do |i|
	    assert_in_delta(Float(i) / 10, b.c[i], 0.001)
	end
    end

    def test_returns_pointer
	setter = lib.find('test_returns_pointer').
	    returns("A*")

	a = setter.call.deference
	assert_equal(10, a.a)
	assert_equal(20, a.b)
	assert_equal(30, a.c)
	assert_equal(40, a.d)
    end

    def test_returns_argument
        wrapper = lib.find("test_returns_argument").
            returns(nil).
            returns("int*")

        assert(! wrapper.return_type)
	assert(wrapper.returns_something?)
	assert_equal([1], wrapper.returned_arguments)
        assert_equal(0, wrapper.arity)

	filtered_args = wrapper.filter
	assert_equal(1, filtered_args.size)

        assert_equal(42, wrapper.call)
	
	wrapper = lib.find('test_ptr_argument_changes').
	    returns(nil).returns('B*')
	b = wrapper.call
	assert(check_B_c_value(b))
	
	wrapper = lib.find('test_arg_input_output').
	    returns(nil).returns("int*").with_arguments('INPUT_OUTPUT_MODE')
	assert_raises(ArgumentError) { wrapper[10, :BOTH] }
	assert_equal(5, wrapper[:OUTPUT])
    end


    def test_modifies_argument
        wrapper = lib.find("test_modifies_argument").
            returns(nil).
            modifies("int*")

        assert(! wrapper.return_type)
	assert(wrapper.returns_something?)
	assert_equal([-1], wrapper.returned_arguments)
        assert_equal(1, wrapper.arity)

	int_value = lib.registry.get("int").new
	int_value.zero!
	assert_equal(0, int_value.to_ruby)

	assert_raises(ArgumentError) { wrapper.filter }
	filtered_args = wrapper.filter(int_value)
	assert_equal(1, filtered_args.size)


        assert_equal(42, wrapper.call(int_value))
	assert_equal(42, int_value.to_ruby)
	
        wrapper = lib.find('test_arg_input_output').
	    modifies("int*").with_arguments('INPUT_OUTPUT_MODE')
        assert_raises(ArgumentError) { wrapper[:OUTPUT] }
        assert_equal(5, wrapper[0, :OUTPUT])
        assert_equal(5, wrapper[10, :BOTH])

	wrapper = lib.find('test_ptr_argument_changes').
	    returns(nil).returns('B*')
	b = wrapper.call
	assert(check_B_c_value(b))

        wrapper = lib.find('test_ptr_argument_changes').
	    modifies('B*')
        b.a.a = 250;
        new_b = wrapper[b]
        # b and new_b are supposed to be the same objects
        assert_equal(b, new_b)
        assert(check_B_c_value(new_b))

        # Check enum I/O handling
        wrapper = lib.find('test_enum_io_handling').
	    modifies('INPUT_OUTPUT_MODE*')
        assert_equal(:BOTH, wrapper[:OUTPUT])
        assert_equal(:OUTPUT, wrapper[:BOTH])
    end

    def test_immediate_to_pointer
        wrapper = lib.find('test_immediate_to_pointer').returns('int').with_arguments('double*')
        assert_equal(1, wrapper.call(0.5))
    end

    
    def test_validation
        # Check that nil is not allowed anywhere but at the return value
        assert_raises(ArgumentError) do
	    lib.find('test_simple_function_findping').
		returns(nil).with_arguments(nil)
	end
        assert_raises(ArgumentError) do
	    lib.find('test_simple_function_findping').
		returns(nil).returns(nil)
	end
        assert_raises(ArgumentError) do
	    lib.find('test_simple_function_findping').
		returns(nil).modifies(nil)
	end
        lib.find('test_ptr_argument_changes').
            returns(nil).with_arguments('B*')
        GC.start

        # NOTE if there is a segfault after the garbage collection, most likely it
        # NOTE because the Ruby findper for Type object has been deleted

        # Check that plain structures aren't allowed
        assert_raises(ArgumentError) do 
	    lib.find('test_simple_function_findping').
		returns("int").
		with_arguments("A")
	end
        assert_raises(ArgumentError) do 
	    lib.find('test_simple_function_findping').
		modifies("int")
	end
        assert_raises(ArgumentError) do 
	    lib.find('test_simple_function_findping').
		returns(nil).
		returns("int")
	end
    end


    def check_argument_passing(wrapper, args = [])
	wrapper.with_arguments('int')
        check = rand(100)
        args  << check

        retcheck, *ret = wrapper[*args]
        assert_equal(check, retcheck)
        return ret
    end

    def test_opaque_handling
        wrapper = lib.find('test_opaque_handling').returns('OPAQUE_TYPE')
        my_handler = wrapper[]

        check_argument_passing lib.find('check_opaque_value').returns('int').with_arguments('OPAQUE_TYPE'), 
	    [my_handler]

	wrapper = lib.find('test_void_argument').returns('int').with_arguments('nil*', 'int')
	arg = lib.registry.get("int").new
	check = rand(100)
	wrapper[arg, check]
	assert_equal(check, arg.to_ruby)
    end

    def test_void_to_ruby
        type = lib.registry.build('void*')
        int  = lib.registry.get("int").new
        assert_raises(TypeError) { type.deference.wrap(int.to_memory_ptr).to_ruby }
    end

    def test_string_handling
        wrapper = lib.find('test_string_argument').with_arguments('char*')
        wrapper["test"]

        wrapper = lib.find('test_string_return').returns('char*')
        assert_equal("string_return", wrapper[].to_str)

        wrapper = lib.find('test_string_argument_modification').
	    with_arguments('char*', 'int')
        buffer = lib.registry.build("char[256]").new
        wrapper[buffer, 256]
        assert_equal("string_return", buffer.to_str)

        wrapper = lib.find('test_string_as_array').
	    returns(nil).modifies('char[256]')
        buffer = lib.registry.build("char[512]").new
        ret = wrapper[buffer]
        assert_kind_of(ArrayType, ret)
        assert_equal("string_return", ret.to_str)
        buffer = " " * 256
        ret = wrapper[buffer]
        assert_kind_of(ArrayType, ret)
        assert_equal("string_return", ret.to_str)

        wrapper = lib.find('test_string_as_array').
	    returns(nil).returns('char[256]')
        retval = wrapper[]
        assert_kind_of(lib.registry.get("char[256]"), retval)
        assert_equal("string_return", retval.to_str)
    end

    def test_compile
        wrapper = lib.find('test_string_argument').returns('int').with_arguments('char*')
        one  = wrapper.compile("test")
        zero = wrapper.compile("bla")

	wrapper["test"]
        one.call
        assert_raises(ArgumentError) { zero.call }
    end
end
end

