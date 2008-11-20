require 'test_config'

class TC_Registry < Test::Unit::TestCase
    def test_aliasing
	registry = Registry.new
	registry.alias "/my_own_and_only_int", "/int"
	assert_equal(registry.get("my_own_and_only_int"), registry.get("int"))
	assert_equal("/int", registry.get("/my_own_and_only_int").name)
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
        assert_nothing_raised {
            registry.import(testfile, nil, :merge => true, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
        }
	assert_raises(RuntimeError) { registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ]) }
    end

    def test_registry_iteration
	reg = make_registry
	values = nil
	assert_nothing_raised { values = reg.enum_for(:each_type).to_a }

	assert_not_equal(0, values.size)
	assert(values.include?(reg.get("/int")))
	assert(values.include?(reg.get("/struct EContainer")))

	assert_nothing_raised { values = reg.enum_for(:each_type, true).to_a }
	assert_not_equal(0, values.size)
	assert(values.include?(["/struct EContainer", reg.get("/struct EContainer")]))
	assert(values.include?(["/EContainer", reg.get("/EContainer")]))
    end

    def test_validate_xml
        test = "malformed_xml"
        assert_raises(ArgumentError) { Typelib::Registry.from_xml(test) }

        test = "<typelib><invalid_element name=\"name\" size=\"0\" /></typelib>"
        assert_raises(ArgumentError) { Typelib::Registry.from_xml(test) }

        test = "<typelib><opaque name=\"name\" /></typelib>"
        assert_raises(ArgumentError) { Typelib::Registry.from_xml(test) }

        test = "<typelib><opaque name=\"invalid type name\" size=\"0\" /></typelib>"
        assert_raises(ArgumentError) { Typelib::Registry.from_xml(test) }
    end
end

    
