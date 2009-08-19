require 'test_config'

class TC_Registry < Test::Unit::TestCase
    Registry = Typelib::Registry
    def test_aliasing
	registry = Registry.new
	registry.alias "/my_own_and_only_int", "/int"
	assert_equal(registry.get("my_own_and_only_int"), registry.get("int"))
	assert_equal("/int", registry.get("/my_own_and_only_int").name)
    end

    def test_guess_type
        assert_raises(RuntimeError) { Registry.guess_type("bla.1") }
        assert_equal("c", Registry.guess_type("blo.c"))
        assert_equal("c", Registry.guess_type("blo.h"))
        assert_equal("tlb", Registry.guess_type("blo.tlb"))
    end

    def test_import_fails_if_files_does_not_exist
        assert_raises(RuntimeError) { Registry.new.import("bla.c") }
    end

    def test_import_raises_if_import_has_errors
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        assert_raises(RuntimeError) { registry.import(testfile) }
    end

    def test_import_include_option
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        registry.import(testfile, nil, :include => [ File.join(SRCDIR, '..') ], :define => [ 'GOOD' ])
    end

    def test_import_raw_option
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
    end

    def test_import_merge
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
        registry.import(testfile, nil, :merge => true, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
    end

    def test_import_fails_if_merge_is_required_but_not_present
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
	registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
	assert_raises(RuntimeError) { registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ]) }
    end

    def make_registry
        registry = Typelib::Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        registry
    end

    def test_resize
        reg = make_registry

        container = reg.get('std/vector</double>')

        test_type = reg.get('StdCollections')
        old_size   = test_type.size
        old_layout = test_type.enum_for(:get_fields).to_a.dup

        assert_not_equal(old_size, 64)
        reg.resize(container => 64)
        new_layout = test_type.enum_for(:get_fields).to_a.dup

        (2...new_layout.size).each do |i|
            assert_equal(new_layout[i][1], old_layout[i][1] + 64 - old_size)
        end
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

    
