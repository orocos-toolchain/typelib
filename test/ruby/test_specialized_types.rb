require 'set'
require 'test_config'
require 'typelib'
require 'test/unit'
require BUILDDIR + '/ruby/libtest_ruby'
require 'pp'

class TC_SpecializedTypes < Test::Unit::TestCase
    include Typelib
    def teardown
	GC.start
    end

    # Not in setup() since we want to make sure
    # that the registry is not destroyed by the GC
    def make_registry
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        registry
    end

    def check_respond_to_fields(a)
        GC.start
        assert( a.respond_to?("a") )
        assert( a.respond_to?("b") )
        assert( a.respond_to?("c") )
        assert( a.respond_to?("d") )
        assert( a.respond_to?("a=") )
        assert( a.respond_to?("b=") )
        assert( a.respond_to?("c=") )
        assert( a.respond_to?("d=") )
    end

    def test_compound_def
        # First, check compound Type objects
        registry = make_registry
        a_type = registry.get("/struct A")
        assert(a_type.respond_to?(:b))
        assert_same(registry.get("/int"), a_type.b)

        # Then, the same on values
        a = a_type.new
        check_respond_to_fields(a)

	b = make_registry.get("/struct B").new
        assert(b.respond_to?(:h))
        assert(b.respond_to?(:h=))
        assert(b.h.kind_of?(Typelib::ArrayType))
        assert(b.respond_to?(:a))
        assert(b.respond_to?(:a=))
        assert(b.a.kind_of?(Typelib::CompoundType))
    end

    def test_compound_init
	a_type = make_registry.get('/struct A')

        # Check initialization
        a = a_type.new :b => 20, :a => 10, :c => 30, :d => 40
        assert( check_struct_A_value(a) )

        a = a_type.new 40, 30, 20, 10
        assert_equal(40, a.a)
        assert_equal(30, a.b)
        assert_equal(20, a.c)
        assert_equal(10, a.d)

	assert_raises(ArgumentError) { a_type.new :b => 10 }
	assert_raises(ArgumentError) { a_type.new 10 }
    end

    def test_compound_get
        a = make_registry.get("/struct A").new
        GC.start
        a = set_struct_A_value(a)
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)

        a = make_registry.get("/struct A").new
        GC.start
        a = set_struct_A_value(a)
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)
    end

    def test_compound_set
        a = make_registry.get("/struct A").new
        GC.start
        a.a = 10;
        a.b = 20;
        a.c = 30;
        a.d = 40;
        assert( check_struct_A_value(a) )
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)
    end

    def test_compound_recursive
        b = make_registry.get("/struct B").new
        GC.start
        assert(b.respond_to?(:a))
        check_respond_to_fields(b.a)

	a = b.a
	assert_same(a, b.a)

        set_struct_A_value(b.a)
        assert_equal(10, b.a.a)
        assert_equal(20, b.a.b)
        assert_equal(30, b.a.c)
        assert_equal(40, b.a.d)

	# Check struct.substruct = Hash
	b.a = { :a => 40, :b => 30, :c => 20, :d => 10 }
        assert_equal(40, b.a.a)
        assert_equal(30, b.a.b)
        assert_equal(20, b.a.c)
        assert_equal(10, b.a.d)
    end

    def test_compound_complex_field_assignment
        b0 = make_registry.get("/struct B").new
        array = make_registry.get("/float[3]").new
        3.times do |i|
            array[i] = i
        end
        b0.f = array

        assert_equal [0, 1, 2], b0.f.to_a

        a = make_registry.get("/struct A").new
        b0.a = a
    end

    def test_array_def
        b = make_registry.get("/struct B").new

	assert_equal(100, b.c.class.length)
        assert_equal('/float[100]', b.c.class.name)
        assert_equal('/float[1]', b.d.class.name)
        assert_equal('/float[1]', b.e.class.name)
        assert_equal('/float[3]', b.f.class.name)
        assert_equal('/float[2]', b.g.class.name)
        assert_equal('/A[4]', b.h.class.name)
        assert_equal('/float[20][10]', b.i.class.name)

        assert_equal(100, b.c.size)
        assert_equal(1, b.d.size)
        assert_equal(1, b.e.size)
        assert_equal(3, b.f.size)
        assert_equal(2, b.g.size)
        assert_equal(4, b.h.size)
        assert_equal(10, b.i.size)
    end

    def test_array_set
        b = make_registry.get("/struct B").new
        (0..(b.c.size - 1)).each do |i|
            b.c[i] = Float(i)/10.0
        end
        check_B_c_value(b)
    end

    def test_array_get
        b = make_registry.get("/struct B").new
        set_B_c_value(b)
        (0..(b.c.size - 1)).each do |i|
	    assert_in_delta(Float(i) / 10.0, b.c[i], 0.01)
        end
	assert_raises(ArgumentError) { b.c[0, 10, 15] }
	assert_raises(IndexError) { b.c[b.c.size + 1] }

	v = b.c[0, 10]
	assert_kind_of(Array, v)
        (0..(v.size - 1)).each do |i|
	    assert_in_delta(Float(i) / 10.0, v[i], 0.01)
        end

	assert_nothing_raised { b.c[b.c.size - 1] }
	assert_raises(IndexError) { b.c[b.c.size - 1, 2] }

	v = b.c[1, 10]
	assert_kind_of(Array, v)
        (0..(v.size - 1)).each do |i|
	    assert_in_delta(Float(i + 1) / 10.0, v[i], 0.01)
        end
    end

    def test_array_each
	registry = make_registry
        b = registry.get("/struct B").new
        set_B_c_value(b)
	
	# Test arrays of arrays
	array = registry.build('float[10][20]').new
	assert_kind_of(Typelib::ArrayType, array)
	array.each do |el|
	    assert_kind_of(registry.get('float[10]'), el)
	end

	# Test arrays of Ruby types
        b.c.each_with_index do |v, i|
	    assert_kind_of(Float, v)
            assert( ( v - Float(i)/10.0 ).abs < 0.01 )
        end

	## Test arrays of Type types
	b.h.each do |el|
	    assert_kind_of(registry.get('struct A'), el)
	end
    end

    def test_array_multi_dim
	mdarray = make_registry.get('TestMultiDimArray').new.fields

	fill_multi_dim_array(mdarray)
	mdarray.each_with_index do |line, y|
	    assert_equal('/int[10]', line.class.name, y)
	    line.each_with_index do |v, x|
		assert_equal(10 * y + x, v)
	    end
	end

	(0..9).each do |y|
	    assert_equal(y * 10, mdarray[y][0])
	    assert_equal(y * 10 + 1, mdarray[y][1])

	    (0..9).each do |x|
		assert_equal(10 * y + x, mdarray[y][x])
	    end
	end
    end

    def test_enum
        registry = make_registry
        e_type = registry.get("EContainer")
        assert(e_type.respond_to?(:value))
        enum = e_type.value
        assert_equal([["E_FIRST", 0], ["E_SECOND", 1], ["E_SET", -1], ["E_PARENS", -2],
                      ["E_OCT", 7],   ["E_HEX", 255],  ["LAST", 8],   ["E_FROM_SIZEOF_STD", 4],
                      ["E_FROM_SIZEOF_SPEC", registry.get("B").size],
                      ["E_FROM_SYMBOL", 255]].to_set, enum.keys.to_set)

        e = e_type.new
        assert(e.respond_to?(:value))
        assert(e.respond_to?(:value=))
        e.value = 0
        assert_equal(:E_FIRST, e.value)
        e.value = "E_FIRST"
        assert_equal(:E_FIRST, e.value)
        e.value = :E_SECOND
        assert_equal(:E_SECOND, e.value)
    end

    def test_numeric
	long = make_registry.get("/int")
	assert(long < NumericType)
	assert(long.integer?)
	assert(!long.unsigned?)
	assert_equal(4, long.size)

	ulong = make_registry.get("/unsigned int")
	assert(ulong < NumericType)
	assert_equal(4, ulong.size)
	assert(ulong.integer?)
	assert(ulong.unsigned?)

	double = make_registry.get("/double")
	assert(double < NumericType)
	assert_equal(8, double.size)
	assert(!double.integer?)
	assert_raises(ArgumentError) { double.unsigned? }
    end

    def test_string_handling
	char_pointer  = make_registry.build("char*").new
	assert(char_pointer.string_handler?)
	assert(char_pointer.respond_to?(:to_str))
    end

    def test_null
        null = make_registry.get("/void")
        assert(null.null?)
    end

    def test_containers
        std = make_registry.get("StdCollections")
        assert(std.dbl_vector < Typelib::ContainerType)
        assert_equal("/std/vector", std.dbl_vector.container_kind)

        value = std.new
        assert_equal(0, value.dbl_vector.length)
        assert(value.dbl_vector.empty?)

        value.dbl_vector.insert(10)
        assert_equal(1, value.dbl_vector.length)
        assert_equal([10], value.dbl_vector.to_a)

        expected = [10]
        10.times do |i|
            value.dbl_vector.insert(i)
            assert_equal(i + 2, value.dbl_vector.length)
            expected << i
            assert_equal(expected, value.dbl_vector.to_a)
        end

        expected.delete_if { |v| v < 5 }
        value.dbl_vector.delete_if { |v| v < 5 }
        assert_equal(expected, value.dbl_vector.to_a)

        expected.delete_at(2)
        value.dbl_vector.erase(6)
        assert_equal(expected, value.dbl_vector.to_a)
    end

    def test_container_of_container
        std      = make_registry.get("StdCollections")
        assert(std.v_of_v < Typelib::ContainerType)
        assert(std.v_of_v.deference < Typelib::ContainerType)

        inner_t = std.v_of_v.deference

        value = std.new
        outer = value.v_of_v
        assert_equal(0, outer.length)

        new_element = inner_t.new
        new_element.insert(10)
        outer.insert(new_element)

        assert_equal(1, outer.length)
        elements = outer.to_a
        assert_kind_of(Typelib::ContainerType, elements[0])
        assert_equal([10], elements[0].to_a)
    end

    def test_container_clear
        value = make_registry.get("StdCollections").new

        value.dbl_vector.insert(10)
        assert(!value.dbl_vector.empty?)
        value.dbl_vector.clear
        assert(value.dbl_vector.empty?)
    end

    def test_define_container
        reg = make_registry
        assert_raises(ArgumentError) { reg.define_container("/blabla") }
        cont = reg.define_container "/std/vector", reg.get("DisplayTest")

        assert(cont < Typelib::ContainerType)
        assert_equal("/std/vector", cont.container_kind)
        assert_equal(reg.get("DisplayTest"), cont.deference)
        assert_equal("/std/vector</DisplayTest>", cont.name)
    end

    def test_std_string
        reg   = make_registry
        type  = reg.get("/std/string")
        value = type.new

        assert value.empty?
        assert_equal 0, value.length

        value.insert(?a)
        assert_equal "a", value.to_ruby

        assert_equal "a_string", Typelib.from_ruby("a_string", reg.get("/std/string")).to_ruby
    end

    def test_boolean
        reg = make_registry

        type = reg.get "bool"

        v = Typelib.from_ruby(true, type)
        assert_kind_of type, v
        assert_equal true, v.to_ruby
        

        v = Typelib.from_ruby(false, type)
        assert_kind_of type, v
        assert_equal false, v.to_ruby
    end

    def test_boolean_in_struct
        reg = make_registry

        type = reg.get "BoolHandling"
        value = type.new

        value.value = true
        assert_equal true, value.value

        value.value = false
        assert_equal false, value.value
    end

    def test_char
        reg = make_registry

        type = reg.get "BoolHandling"
    end
end

