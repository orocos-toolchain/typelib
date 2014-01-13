require './test_config'
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

	values = Typelib.log_silent { reg.each.to_a }
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
        Typelib.log_silent do
            reg.export_to_ruby(mod)
        end

        assert_equal reg.get('/E_comparison_1/E_with_added_values'),
            mod::EComparison_1::EWithAddedValues
        assert_equal reg.get('/B'),
            mod::B

    end

    def test_clear_export
	reg = make_registry
        mod = Module.new
        Typelib.log_silent do
            reg.export_to_ruby(mod)
        end

        ns = mod.const_get(:NS1).const_get(:NS2)
        assert(!ns.constants.empty?)
        assert(!ns.exported_types.empty?, "#{ns.exported_types.inspect} was expected to be empty")

        reg.clear_exports(mod)

        assert_equal([:EComparison_1, :EComparison_2, :NS1, :VeryLongNamespaceName, :Std].to_set, mod.constants.map(&:to_sym).to_set)
        assert(mod.exported_types.empty?, "#{mod.exported_types.inspect} was expected to be empty")

        assert(ns.constants.empty?)
        assert(ns.exported_types.empty?, "#{ns.exported_types.inspect} was expected to be empty")
    end

    def test_clear_export_keeps_custom_objects
	reg = make_registry
        mod = Module.new
        Typelib.log_silent do
            reg.export_to_ruby(mod)
        end

        obj = Object.new
        mod.const_set('CustomConstant', obj)
        reg.clear_exports(mod)
        assert_equal([:EComparison_1, :EComparison_2, :NS1, :VeryLongNamespaceName, :Std, :CustomConstant].to_set, mod.constants.map(&:to_sym).to_set)
        assert(mod.exported_types.empty?, "#{mod.exported_types.inspect} was expected to be empty")
        assert_equal(obj, mod::CustomConstant)
    end

    def test_export_converted_type
	reg = make_registry
        mod = Module.new

        converted_t = reg.get('/B')
        converted_t.convert_to_ruby(Time) { |bla| }
        Typelib.log_silent do
            reg.export_to_ruby(mod)
        end

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
        Typelib.log_silent do
            reg.export_to_ruby(mod)
        end

        assert_equal(target_t, mod::EComparison_1::EWithAddedValues)
    end

    def test_create_enum
        reg = make_registry
        t = reg.create_enum('/NewEnum') do |t|
            t.VAL0
            t.VAL1 = -1
            t.VAL2
        end

        assert_equal({'VAL0' => 0, 'VAL1' => -1, 'VAL2' => 0}, t.keys)

        assert_raises(ArgumentError) do
            reg.create_enum('NewEnum') { |t| t.VAL0 }
        end
        assert_raises(ArgumentError) do
            reg.create_enum('/NewEnum') { |t| }
        end
    end

    def test_create_container
        reg = make_registry

        vector_t = reg.create_container('/std/vector', '/int')
        assert(vector_t <= Typelib::ContainerType)
        assert_same(reg.get('int'), vector_t.deference)

        assert_raises(Typelib::NotFound) do
            reg.create_container('/this_is_an_unknown_container', '/int')
        end

        assert_raises(Typelib::NotFound) do
            reg.create_container('/std/vector', '/this_is_an_unknown_type')
        end
    end

    def test_create_array
        reg = make_registry

        array_t = reg.create_array('/int', 20043)
        assert(array_t <= Typelib::ArrayType)
        assert_equal(array_t.length, 20043)
        assert_same(reg.get('int'), array_t.deference)

        assert_raises(Typelib::NotFound) do
            reg.create_array('/this_is_an_unknown_type', 10000)
        end
    end

    def test_create_compound
        reg = make_registry
        assert_raises(ArgumentError) do
            reg.create_compound('NewCompound') { |t| t.field0 = 'int' }
        end
        assert_raises(ArgumentError) do
            reg.create_compound('/NewCompound') { |t| }
        end
        assert_raises(Typelib::NotFound) do
            reg.create_compound('/NewCompound') { |t| t.field0 = 'this_is_an_unknown_type' }
        end
        type = reg.create_compound('/NewCompound') do |t|
            t.field0 = 'int'
            t.add('field1', 'double', 10)
            t.field2 = 'double[29459]'
        end
        assert(type <= Typelib::CompoundType)
        assert_same(reg.get('int'), type['field0'])
        assert_equal(0, type.offset_of('field0'))
        assert_same(reg.get('double'), type['field1'])
        assert_equal(10, type.offset_of('field1'))
        assert_same(reg.get('double[29459]'), type['field2'])
        assert_equal(10 + type['field1'].size, type.offset_of('field2'))
    end

    def test_merge_keeps_metadata
        reg = Typelib::Registry.new
        Typelib::Registry.add_standard_cxx_types(reg)
        type = reg.create_compound '/Test' do |c|
            c.add 'field', 'double'
        end
        type.metadata.set('k', 'v')
        type.field_metadata['field'].set('k', 'v')
        new_reg = Typelib::Registry.new
        new_reg.merge(reg)
        new_type = new_reg.get('/Test')
        assert_equal [['k', ['v']]], new_type.metadata.each.to_a
        assert_equal [['k', ['v']]], new_type.field_metadata['field'].each.to_a
    end

    def test_minimal_keeps_metadata
        reg = Typelib::Registry.new
        Typelib::Registry.add_standard_cxx_types(reg)
        type = reg.create_compound '/Test' do |c|
            c.add 'field', 'double'
        end
        type.metadata.set('k', 'v')
        type.field_metadata['field'].set('k', 'v')
        new_reg = reg.minimal('/Test')
        new_type = new_reg.get('/Test')
        assert_equal [['k', ['v']]], new_type.metadata.each.to_a
        assert_equal [['k', ['v']]], new_type.field_metadata['field'].each.to_a
    end

    def test_create_opaque_raises_ArgumentError_if_the_name_is_already_used
        reg = Typelib::Registry.new
        reg.create_opaque '/Test', 10
        assert_raises(ArgumentError) { reg.create_opaque '/Test', 10 }
    end

    def test_create_null_raises_ArgumentError_if_the_name_is_already_used
        reg = Typelib::Registry.new
        reg.create_null '/Test'
        assert_raises(ArgumentError) { reg.create_null '/Test' }
    end

    def test_reverse_depends_resolves_recursively
        reg = Typelib::Registry.new
        Typelib::Registry.add_standard_cxx_types(reg)
        compound_t = reg.create_compound '/C' do |c|
            c.add 'field', 'double'
        end
        vector_t = reg.create_container '/std/vector', compound_t
        array_t  = reg.create_array vector_t, 10
        assert_equal [compound_t, array_t, vector_t].to_set,
            reg.reverse_depends(compound_t).to_set
    end

    def test_remove_removes_the_types_and_its_dependencies
        reg = Typelib::Registry.new
        Typelib::Registry.add_standard_cxx_types(reg)
        compound_t = reg.create_compound '/C' do |c|
            c.add 'field', 'double'
        end
        vector_t = reg.create_container '/std/vector', compound_t
        reg.create_array vector_t, 10
        reg.remove(compound_t)
        assert !reg.include?("/std/vector</C>")
        assert !reg.include?("/std/vector</C>[10]")
    end
end

    
