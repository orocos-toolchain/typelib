require 'test_config'
require 'typelib'
require 'test/unit'

class TC_IDL < Test::Unit::TestCase
    include Typelib

    def test_export_validation
	test_file = File.join(SRCDIR, "data", "test_idl.h")

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

    def check_export(input_name, output_name = input_name, options = {})
	registry = Registry.new
	registry.import( File.join(SRCDIR, "data", "#{input_name}.h"), "c" )
	output = if block_given?
		    yield
		else
		    registry.export("idl", options)
		end

	expected = File.read(File.join(SRCDIR, "data", "#{output_name}.idl"))
	assert(expected == output, "output: #{output}\nexpected: #{expected}")
    end

    def test_export_output
	check_export("test_idl")
	check_export("test_idl", "test_idl_prefix_suffix", 
		     :namespace_prefix => 'CorbaPrefix/TestPrefix', 
		     :namespace_suffix => 'CorbaSuffix/TestSuffix')

	check_export("laser", "laser", :namespace_suffix => 'Corba')
    end
end

