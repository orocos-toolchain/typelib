require 'set'
require 'test_config'
require 'typelib'
require 'test/unit'
require '.libs/test_rb_value'
require 'pp'

class TC_Value < Test::Unit::TestCase
    include Typelib

    # Not in setup() since we want to make sure
    # that the registry is not destroyed by the GC
    def make_registry
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        registry
    end

    def test_registry
        assert_raises(RuntimeError) { Registry.guess_type("bla.1") }
        assert_equal("c", Registry.guess_type("blo.c"))
        assert_equal("c", Registry.guess_type("blo.h"))
        assert_equal("tlb", Registry.guess_type("blo.tlb"))

        assert_raises(RuntimeError) { Registry.new.import("bla.c") }

        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        assert_raises(RuntimeError) { registry.import(testfile) }
        assert_nothing_raised {
            registry.import(testfile, nil, :include => [ File.join(SRCDIR, '..') ], :define => [ 'GOOD' ])
        }

        registry = Registry.new
        assert_nothing_raised {
            registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
        }
    end

    def test_basic_behaviour
        # Check that == returns false when the two objects aren't of the same class
        # (for instance type == nil shall return false)
        type = Registry.new.get("/int")
        assert_equal("/int", type.name)
        assert_nothing_raised { type == nil }
        assert_nothing_raised { type != nil }
    end

    def test_pointer_type
        type = Registry.new.build("/int*")
        assert_not_equal(type, type.deference)
        assert_not_equal(type, type.to_ptr)
        assert_not_equal(type.to_ptr, type.deference)
        assert_equal(type, type.deference.to_ptr)
        assert_equal(type, type.to_ptr.deference)

	value = type.new
	value.zero!
	assert(value.null?)
	assert_equal(nil, value.to_ruby)
    end
    def test_pointer_value
        type  = Registry.new.build("/int")
        value = type.new
        assert_equal(value.class, type)
        ptr   = value.to_ptr
        assert_equal(value, ptr.deference)
    end

    def test_string_handling
        buffer_t = Registry.new.build("/char[256]")
        buffer = buffer_t.new
        
        # Check that .from_string.to_string is an identity
        assert_equal("first test", buffer.from_string("first test").to_string)
        assert_raises(TypeError) { buffer.from_string("a"*512) }
    end

    def test_import
        registry = make_registry
        assert( registry.get("/struct A") )
        assert( registry.get("/ADef") )
    end

    def test_compound
        # First, check compound Type objects
        registry = make_registry
        a_type = registry.get("/struct A")
        assert(a_type.respond_to?(:b))
        assert(a_type.b == registry.get("/long"))

        # Then, the same on values
        a = a_type.new
        GC.start
        check_respond_to_fields(a)

        # Check initialization
        a = a_type.new :a => 10, :b => 20, :c => 30, :d => 40
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)

        a = a_type.new 40, 30, 20, 10
        assert_equal(40, a.a)
        assert_equal(30, a.b)
        assert_equal(20, a.c)
        assert_equal(10, a.d)

	assert_raises(ArgumentError) { a_type.new :b => 10 }
	assert_raises(ArgumentError) { a_type.new 10 }
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

    def test_get
        a = make_registry.get("/struct A").new
        GC.start
        a = set_struct_A_value(a)
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)
    end

    def test_set
        a = make_registry.get("/struct A").new
        GC.start
        a.a = 10;
        a.b = 20;
        a.c = 30;
        a.d = 40;
        assert( check_struct_A_value(a) )
    end

    def test_recursive
        b = make_registry.get("/struct B").new
        GC.start
        assert(b.respond_to?(:a))
        assert(! b.respond_to?(:a=))
        check_respond_to_fields(b.a)

        set_struct_A_value(b.a)
        assert_equal(10, b.a.a)
        assert_equal(20, b.a.b)
        assert_equal(30, b.a.c)
        assert_equal(40, b.a.d)
    end

    def test_array_basics
        b = make_registry.get("/struct B").new
        GC.start
        assert(b.respond_to?(:c))
        assert(! b.respond_to?(:c=))
        assert(b.c.kind_of?(Typelib::ArrayType))
        assert_equal(100, b.c.size)
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
        b = make_registry.get("/struct B").new
        set_B_c_value(b)
        b.c.each_with_index do |v, i|
            assert( ( v - Float(i)/10.0 ).abs < 0.01 )
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

