require 'test/unit'
require 'test_config'
require '.libs/test_rb_value'
require 'typelib'
require 'pp'

class TC_Value < Test::Unit::TestCase
    include Typelib
    attr_reader :registry

    def setup
        @registry = Registry.new

        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )
    end

    def test_import
        assert( registry.get("/struct A") )
        assert( registry.get("/ADef") )
    end

    def test_respond_to
        a = Value.new(nil, registry.get("/struct A"))
        check_respond_to_fields(a)
    end

    def check_respond_to_fields(a)
        assert( a.respond_to?("a") )
        assert( a.respond_to?("b") )
        assert( a.respond_to?("c") )
        assert( a.respond_to?("d") )
        assert( a.respond_to?("a=") )
        assert( a.respond_to?("b=") )
        assert( a.respond_to?("c=") )
        assert( a.respond_to?("d=") )
    end

    def test_value_get
        a = Value.new(nil, registry.get("/struct A"))
        a = set_struct_A_value(a)
        assert_equal(10, a.a)
        assert_equal(20, a.b)
        assert_equal(30, a.c)
        assert_equal(40, a.d)
    end

    def test_value_set
        a = Value.new(nil, registry.get("/struct A"))
        a.a = 1;
        a.b = 2;
        a.c = 3;
        a.d = 4;
        assert( check_struct_A_value(a) )
    end

    def test_value_complex
        b = Value.new(nil, registry.get("/struct B"))
        assert(b.respond_to?(:a))
        assert(! b.respond_to?(:a=))
        check_respond_to_fields(b.a)

        set_struct_A_value(b.a)
        assert_equal(10, b.a.a)
        assert_equal(20, b.a.b)
        assert_equal(30, b.a.c)
        assert_equal(40, b.a.d)
    end

end

