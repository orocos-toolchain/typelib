require 'test/unit'
require 'test_config'
require 'typelib'

class TC_Value < Test::Unit::TestCase
    include Typelib

    def setup
        reg = Registry.new

        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { reg.import( testfile  ) }
        reg.import( testfile, "c" )
    end

    def test_import
        assert( reg.get("/struct A") )
        assert( reg.get("/ADef") )
    end

    def test_set_value
        a = Value.new(nil, reg.get("/struct A"))
        a.a = 1;
        a.b = 2;
        a.c = 3;
        a.d = 4;
        assert( check_struct_A_value(a) )
    end

    def test_get_value
    end
end

