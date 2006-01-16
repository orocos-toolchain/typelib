require 'test/unit'
require 'test_config'
require 'typelib'

class TC_Value < Test::Unit::TestCase
    include Typelib

    def test_import
        reg = Registry.new

        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { reg.import( testfile  ) }
        reg.import( testfile, "c" )

        assert( reg.get("/struct A") )
        assert( reg.get("/ADef") )
    end
end

