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
        a_type = registry.get("/A")
        a = a_type.new
        check_respond_to_fields(a)

	b = make_registry.get("/B").new
        assert(b.respond_to?(:h))
        assert(b.respond_to?(:h=))
        assert(b.h.kind_of?(Typelib::ArrayType))
        assert(b.respond_to?(:a))
        assert(b.respond_to?(:a=))
        assert(b.a.kind_of?(Typelib::CompoundType))
    end

    def test_compound_init
	a_type = make_registry.get('/A')

        # Check initialization
        a = a_type.new :b => 20, :a => 10, :c => 30, :d => 40
        assert( check_struct_A_value(a) )

        a = a_type.new [40, 30, 20, 10]
        assert_equal(40, a.a)
        assert_equal(30, a.b)
        assert_equal(20, a.c)
        assert_equal(10, a.d)
    end

    def test_compound_get
        a = make_registry.get("/A").new
        GC.start
        a = set_struct_A_value(a)
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)

        a = make_registry.get("/A").new
        GC.start
        a = set_struct_A_value(a)
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)
    end

    def test_compound_set
        a = make_registry.get("/A").new
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
        b = make_registry.get("/B").new
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
        b0 = make_registry.get("/B").new
        array = make_registry.get("/float[3]").new
        3.times do |i|
            array[i] = i
        end
        b0.f = array

        assert_equal [0, 1, 2], b0.f.to_a

        a = make_registry.get("/A").new
        b0.a = a
    end

    def test_compound_method_overloading
        registry = make_registry
        t = registry.get("/CompoundWithOverloadingClashes")
        v = t.new
        v.zero!
        assert_equal 0, v.name
        assert_equal v, v.cast(t)
        assert_equal 0, v.object_id
    end

    def test_array_def
        b = make_registry.get("/B").new

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
        b = make_registry.get("/B").new
        (0..(b.c.size - 1)).each do |i|
            b.c[i] = Float(i)/10.0
        end
        check_B_c_value(b)
    end

    def test_array_get
        b = make_registry.get("/B").new
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
        b = registry.get("/B").new
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
	    assert_kind_of(registry.get('A'), el)
	end
    end

    def test_array_multi_dim
        registry = make_registry
	mdarray = registry.get('TestMultiDimArray').new.fields

        int_t = registry.get("/int")

	fill_multi_dim_array(mdarray)
	mdarray.each_with_index do |line, y|
	    assert_equal("#{int_t.name}[10]", line.class.name, y)
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
        enum   = e_type[:value]
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

    def test_enum_can_cast_to_superset
        registry = make_registry
        e_type  = registry.get("E")
        e_added = registry.get("E_comparison_1/E_with_added_values")
        assert(!(e_type == e_added))
        assert(e_type.casts_to?(e_added))
    end

    def test_enum_cannot_cast_to_subset
        registry = make_registry
        e_type  = registry.get("E")
        e_added = registry.get("E_comparison_1/E_with_added_values")
        assert(!(e_type == e_added))
        assert(!(e_added.casts_to?(e_type)))
    end

    def test_enum_cannot_cast_to_modified
        registry = make_registry
        e_type  = registry.get("E")
        e_modified = registry.get("E_comparison_2/E_with_modified_values")
        assert(!(e_type == e_modified))
        assert(!(e_modified.casts_to?(e_type)))
    end

    def test_numeric
	long = make_registry.get("/int")
	assert(long < NumericType)
	assert(long.integer?)
	assert(!long.unsigned?)
	assert_equal(4, long.size)

        long_v = long.from_ruby(10)
        assert_equal 10, long_v.to_ruby

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
        assert(std[:dbl_vector] < Typelib::ContainerType)
        assert_equal("/std/vector", std[:dbl_vector].container_kind)

        value = std.new
        assert_equal(0, value.dbl_vector.length)
        assert(value.dbl_vector.empty?)

        value.dbl_vector.push(10)
        assert_equal(1, value.dbl_vector.length)
        assert_equal([10], value.dbl_vector.to_a)

        expected = [10]
        10.times do |i|
            value.dbl_vector.push(i)
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

    def test_container_random_access
        std = make_registry.get("StdCollections")
        value = std.new
        value.dbl_vector.push(20)
        assert_equal 20, value.dbl_vector[0]
        value.dbl_vector[0] = 10
        assert_equal 10, value.dbl_vector[0]
    end

    def test_container_of_container
        std      = make_registry.get("StdCollections")
        assert(std[:v_of_v] < Typelib::ContainerType)
        assert(std[:v_of_v].deference < Typelib::ContainerType)

        inner_t = std[:v_of_v].deference

        value = std.new
        outer = value.v_of_v
        assert_equal(0, outer.length)

        new_element = inner_t.new
        new_element.push(10)
        outer.push(new_element)
        new_element.push(20)
        outer.push(new_element)

        assert_equal(2, outer.length)
        elements = outer.to_a
        assert_kind_of(Typelib::ContainerType, elements[0])
        assert_equal([10], elements[0].to_a)
        assert_kind_of(Typelib::ContainerType, elements[1])
        assert_equal([10, 20], elements[1].to_a)
    end

    def test_container_clear
        value = make_registry.get("StdCollections").new

        value.dbl_vector.push(10)
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

        value.push(?a)
        assert_equal "a", Typelib.to_ruby(value)
        assert_equal "a_string", Typelib.to_ruby(Typelib.from_ruby("a_string", reg.get("/std/string")))
    end

    def test_std_string_push
        reg   = make_registry
        string_t  = reg.get("/std/string")

        str = Typelib.from_ruby("string", string_t)
        str << "1"
        assert_equal "string1", Typelib.to_ruby(str)

        assert_raises(ArgumentError) { str << "longer" }
        assert_raises(ArgumentError) { str << "" }
    end

    def test_std_string_concat
        reg   = make_registry
        string_t  = reg.get("/std/string")

        str = Typelib.from_ruby("string1", string_t)
        str.concat("string2")
        assert_equal "string1string2", Typelib.to_ruby(str)
    end

    def test_boolean
        reg = make_registry

        type = reg.get "bool"

        v = Typelib.from_ruby(true, type)
        assert_kind_of type, v
        assert_equal true, Typelib.to_ruby(v, type)

        v = Typelib.from_ruby(false, type)
        assert_kind_of type, v
        assert_equal false, Typelib.to_ruby(v, type)
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

    def test_vector_freeze
        vector_t = make_registry.get("/std/vector</double>")
        vector = vector_t.new

        10.times do |i|
            vector.push(i)
        end
        vector.freeze
        assert(vector.frozen?)
        assert_raises(TypeError) { vector.push(10) }
        assert_raises(TypeError) { vector.erase(10) }
        assert_raises(TypeError) { vector.delete_if { } }
        assert_raises(TypeError) { vector[0] = 10 }
        assert_equal(5, vector[5])
    end

    def test_vector_invalidate_refuses_toplevel_values
        vector_t = make_registry.get("/std/vector</double>")
        vector = vector_t.new
        assert_raises(ArgumentError) { vector.invalidate }
    end

    def test_vector_invalidate
        main_t = make_registry.get("StdCollections")
        main = main_t.new
        vector = main.dbl_vector

        10.times do |i|
            vector.push(i)
        end
        vector.invalidate
        assert(vector.invalidated?)
        assert_raises(TypeError) { vector.push(10) }
        assert_raises(TypeError) { vector.erase(10) }
        assert_raises(TypeError) { vector.delete_if { } }
        assert_raises(TypeError) { vector[0] }
        assert_raises(TypeError) { vector[0] = 10 }
    end

    def test_vector_modification_invalidate_existing_values
        std      = make_registry.get("StdCollections")
        value_t = std[:v_of_v]

        value   = value_t.new
        element = value_t.deference.new
        10.times do
            value.push(element)
        end

        v = value[0]
        assert(!v.invalidated?)
        value.push(element)
        assert(v.invalidated?)

        v = value[0]
        assert(!v.invalidated?)
        value.erase(element)
        assert(v.invalidated?)

        10.times do
            value.push(element)
        end
        v = value[0]
        assert(!v.invalidated?)
        bool = false
        value.delete_if { bool = !bool }
        assert(v.invalidated?)
    end
end

