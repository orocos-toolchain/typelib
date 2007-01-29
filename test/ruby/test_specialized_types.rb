require 'set'
require 'test_config'
require 'typelib'
require 'test/unit'
require '.libs/test_rb_value'
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
        assert(a_type.b == registry.get("/long"))

        # Then, the same on values
        a = a_type.new
        check_respond_to_fields(a)

	b = make_registry.get("/struct B").new
        assert(b.respond_to?(:h))
        assert(! b.respond_to?(:h=))
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
	
    end

    def test_compound_recursive
        b = make_registry.get("/struct B").new
        GC.start
        assert(b.respond_to?(:a))
        check_respond_to_fields(b.a)

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

    def test_array_def
        b = make_registry.get("/struct B").new

        assert_equal('/float[100]', b.c.class.name)
        assert_equal('/float[1]', b.d.class.name)
        assert_equal('/float[1]', b.e.class.name)
        assert_equal('/float[3]', b.f.class.name)
        assert_equal('/float[2]', b.g.class.name)
        assert_equal('/struct A[4]', b.h.class.name)
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
            assert( ( b.c[i] - Float(i)/10.0 ).abs < 0.01 )
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
	    assert_equal(y * 10 + 1, mdarray[y][1])

	    (0..9).each do |x|
		assert_equal(10 * y + x, mdarray[y][x])
	    end
	end
    end

    def test_enum
        e_type = make_registry.get("EContainer")
        assert(e_type.respond_to?(:value))
        enum = e_type.value
        assert_equal([["FIRST", 0], ["SECOND", 1], ["THIRD", -1], ["FOURTH", -2], ["LAST", -1]].to_set, enum.keys.to_set)

        e = e_type.new
        assert(e.respond_to?(:value))
        assert(e.respond_to?(:value=))
        e.value = 0
        assert_equal(:FIRST, e.value)
        e.value = "FIRST"
        assert_equal(:FIRST, e.value)
        e.value = :SECOND
        assert_equal(:SECOND, e.value)
    end

end

