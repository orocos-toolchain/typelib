require 'test_config'
require 'typelib'
require 'test/unit'

class TC_IDL < Test::Unit::TestCase
    include Typelib

    def test_export_validation
	test_file = File.join(SRCDIR, "data", "test_idl.h")

        registry = Registry.new
        registry.import( test_file, "c", :define => 'IDL_POINTER_ALIAS' )
	assert_raises(RuntimeError) { registry.export("idl") }

        registry = Registry.new
        registry.import( test_file, "c", :define => 'IDL_POINTER_IN_STRUCT' )
	assert_raises(RuntimeError) { registry.export("idl") }

        registry = Registry.new
        registry.import( test_file, "c", :define => 'IDL_MULTI_ARRAY' )
	assert_raises(RuntimeError) { registry.export("idl") }
    end

    def verify_expected_idl(output, expected_filename)
        expected = File.read(expected_filename)
        if expected != output
            File.open('output.idl', 'w') { |io| io.write(output) }
        end
	assert(expected == output, "generated and expected IDL mismatches. Expected IDL is in #{expected_filename}, generated is in output.idl")
    end

    def check_export(input_name, output_name = input_name, options = {})
	registry = Registry.new
	registry.import( File.join(SRCDIR, "data", "#{input_name}.h"), "c" )
	output = if block_given?
		    yield
		else
		    registry.export("idl", options)
		end

        verify_expected_idl(output, File.join(SRCDIR, "data", "#{output_name}.idl"))
    end

    def test_export_output
	check_export("test_idl")
	check_export("test_idl", "test_idl_prefix_suffix", 
		     :namespace_prefix => 'CorbaPrefix/TestPrefix', 
		     :namespace_suffix => 'CorbaSuffix/TestSuffix')

	check_export("laser", "laser", :namespace_suffix => 'Corba')
    end

    def test_blob_threshold
	check_export("laser", "laser_blobs", :blob_threshold => "1024")
    end
end

