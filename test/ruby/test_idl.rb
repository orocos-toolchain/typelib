require 'test_config'
require 'typelib'
require 'test/unit'

class TC_IDL < Test::Unit::TestCase
    include Typelib

    attr_reader :test_file
    def setup
        @test_file = File.join(SRCDIR, "idl_export.h")
    end

    def test_export
        registry = Registry.new
        registry.import( test_file, "c" )
	value = nil
	assert_nothing_raised { value = registry.export("idl") }
	expected = File.read(File.join(SRCDIR, "idl_export.idl"))
	assert_equal(expected, value)

	assert_nothing_raised do 
	    value = registry.export("idl", 
		    :namespace_prefix => 'CorbaPrefix/TestPrefix', 
		    :namespace_suffix => 'CorbaSuffix/TestSuffix')
	end
	expected = File.read(File.join(SRCDIR, "idl_export_prefix_suffix.idl"))
	assert_equal(expected, value)

        registry = Registry.new
        registry.import( test_file, "c", :define => 'IDL_POINTER_ALIAS' )
	assert_raises(TypeError) { registry.export("idl") }

        registry = Registry.new
        registry.import( test_file, "c", :define => 'IDL_POINTER_IN_STRUCT' )
	assert_raises(TypeError) { registry.export("idl") }

        registry = Registry.new
        registry.import( test_file, "c", :define => 'IDL_MULTI_ARRAY' )
	assert_raises(TypeError) { registry.export("idl") }
    end
end

