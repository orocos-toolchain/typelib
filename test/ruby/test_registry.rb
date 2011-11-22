require 'test_config'
require 'set'

class TC_Registry < Test::Unit::TestCase
    Registry = Typelib::Registry
    CXXRegistry = Typelib::CXXRegistry
    def test_aliasing
	registry = CXXRegistry.new
	registry.alias "/my_own_and_only_int", "/int"
        current_aliases = registry.aliases_of(registry.get("int"))
	assert_equal(registry.get("my_own_and_only_int"), registry.get("int"))

	assert_equal((current_aliases.to_set << "/my_own_and_only_int"), registry.aliases_of(registry.get("/int")).to_set)
    end

    def test_guess_type
        assert_raises(RuntimeError) { Registry.guess_type("bla.1") }
        assert_equal("c", Registry.guess_type("blo.c"))
        assert_equal("c", Registry.guess_type("blo.h"))
        assert_equal("tlb", Registry.guess_type("blo.tlb"))
    end

    def test_import_fails_if_files_does_not_exist
        assert_raises(ArgumentError) { Registry.import("bla.c") }
    end

    def test_import_raises_if_import_has_errors
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        assert_raises(ArgumentError) { registry.import(testfile) }
    end

    def test_import_include_option
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        registry.import(testfile, nil, :include => [ File.join(SRCDIR, '..') ], :define => [ 'GOOD' ])
    end

    def test_import_raw_option
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        registry.import(testfile, nil, :rawflags => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
    end

    def test_import_merge
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        registry.import(testfile, nil, :rawflags => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
        registry.import(testfile, nil, :merge => true, :rawflags => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
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

	values = reg.each.to_a
	assert_not_equal(0, values.size)
	assert(values.include?(reg.get("/int")))
	assert(values.include?(reg.get("/EContainer")))

	values = reg.each(:with_aliases => true).to_a
	assert_not_equal(0, values.size)
	assert(values.include?(["/EContainer", reg.get("/EContainer")]))

	values = reg.each('/NS1').to_a
	assert_equal(6, values.size, values.map(&:name))
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

    def test_export
	reg = make_registry
        mod = Module.new
        reg.export_to_ruby(mod)

        assert_equal reg.get('/E_comparison_1/E_with_added_values'),
            mod::EComparison_1::EWithAddedValues
        assert_equal reg.get('/B'),
            mod::B

    end

    def test_clear_export_keeps_custom_objects
	reg = make_registry
        mod = Module.new
        reg.export_to_ruby(mod)

        obj = Object.new
        mod.const_set('CustomConstant', obj)
        reg.clear_exports(mod)
        assert_equal([:EComparison_1, :EComparison_2, :NS1, :VeryLongNamespaceName, :Std, :CustomConstant].to_set, mod.constants.to_set)
        assert(mod.exported_types.empty?, "#{mod.exported_types.inspect} was expected to be empty")
        assert_equal(obj, mod::CustomConstant)
    end

    def test_export_converted_type
	reg = make_registry
        mod = Module.new

        converted_t = reg.get('/B')
        converted_t.convert_to_ruby(Time) { |bla| }
        reg.export_to_ruby(mod)

        assert_equal(Time, mod::B)
    end

    def test_export_converted_type_to_existing_type
	reg = make_registry
        mod = Module.new
        target_t = Class.new

        ns = Module.new
        mod.const_set(:EComparison_1, ns)
        ns.const_set(:EWithAddedValues, target_t)

        converted_t = reg.get('/E_comparison_1/E_with_added_values')
        converted_t.convert_to_ruby(target_t) { |bla| }
        reg.export_to_ruby(mod)

        assert_equal(target_t, mod::EComparison_1::EWithAddedValues)
    end
end

    
