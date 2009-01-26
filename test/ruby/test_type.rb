require 'set'
require 'test_config'
require 'typelib'
require 'test/unit'
require BUILDDIR + '/ruby/libtest_ruby'
require 'pp'

class TC_Type < Test::Unit::TestCase
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

    def test_type_inequality
        # Check that == returns false when the two objects aren't of the same class
        # (for instance type == nil shall return false)
	type = nil
        type = Registry.new.get("/int")
        assert_equal("/int", type.name)
        assert_nothing_raised { type == nil }
        assert(type != nil)
    end

    def test_type_names
	registry = make_registry
	bla = registry.get "/NS1/Bla/Test"
	assert_equal("/NS1/Bla/Test", bla.name)
	assert_equal("Test", bla.basename)
	assert_equal("/NS1/Bla/", bla.namespace)
	assert_equal("::NS1::Bla::", bla.namespace('::'))
	assert_equal("/NS1/Bla/Test", bla.full_name)
	assert_equal("::NS1::Bla::Test", bla.full_name('::'))
	assert_equal("NS1/Bla/Test", bla.full_name(Typelib::NAMESPACE_SEPARATOR, true))
	assert_equal("NS1::Bla::Test", bla.full_name('::', true))

        test_data = "<typelib><opaque name=\"/Bla/Blo&lt;/Template&gt;\" size=\"0\" /></typelib>"
        registry = Registry.from_xml(test_data)
        type     = registry.get("/Bla/Blo</Template>")
        assert_equal("/Bla/", type.namespace)
        assert_equal("Blo</Template>", type.basename)
        assert_equal("::Bla::Blo<::Template>", type.full_name("::", false))
    end

    def test_pointer
        type = Registry.new.build("/int*")
        assert_not_equal(type, type.deference)
        assert_not_equal(type, type.to_ptr)
        assert_not_equal(type.to_ptr, type.deference)
        assert_equal(type, type.deference.to_ptr)
        assert_equal(type, type.to_ptr.deference)
    end

    def test_memory_layout
        reg = make_registry
        std = reg.get("StdCollections")
        off_dlb_vector = std.offset_of("dbl_vector")
        off_v8 = std.offset_of("v8")
        off_v_of_v = std.offset_of("v_of_v")

        layout = std.memory_layout
        expected = [:FLAG_MEMCPY, off_dlb_vector,
            :FLAG_CONTAINER, reg.get("/std/vector</double>"),
                :FLAG_MEMCPY, 8,
            :FLAG_END,
            :FLAG_MEMCPY, off_v_of_v - off_v8,
            :FLAG_CONTAINER, reg.get("/std/vector</std/vector</double>>"),
                :FLAG_CONTAINER, reg.get("/std/vector</double>"),
                    :FLAG_MEMCPY, 8,
                :FLAG_END,
            :FLAG_END,
            :FLAG_MEMCPY, 16]

        assert_equal(expected, layout)
    end
end


