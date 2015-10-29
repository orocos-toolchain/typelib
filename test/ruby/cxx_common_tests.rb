# Common tests for all C++ importers
#
# The module is meant to be included in a test class, which must set the @loader
# instance variable with a {Typelib::CXX}-compatible loader object, and may
# update the importer_options hash with default options that must be passed to
# the loader
module CXXCommonTests
    attr_reader :loader, :importer_options

    def setup
        super
        @importer_options ||= Hash.new
    end

    # The bulk of the C++ tests are made of a C++ file and an expected tlb file.
    # This method generate one test method per such file
    #
    # @param [String] dir the directory containing the tests
    def self.generate_common_tests(dir)
        singleton_class.class_eval do
            define_method(:cxx_test_dir) { dir }
        end

        Dir.glob(File.join(dir, '*.hh')) do |file|
            basename = File.basename(file, '.hh')
            prefix   = File.join(dir, basename)
            opaques  = "#{prefix}.opaques"
            tlb      = "#{prefix}.tlb"
            next if !File.file?(tlb)

            define_method "test_cxx_common_#{basename}" do
                reg = Typelib::Registry.new
                expected = Typelib::Registry.from_xml(File.read(tlb))
                if File.file?(opaques)
                    reg.import(opaques, 'tlb')
                end
                loader.load(reg, file, 'c', importer_options)

                expected.each(with_aliases: true) do |name, expected_type|
                    actual_type = reg.build(name)

                    if expected_type != actual_type
                        error_message = "in #{file}, failed expected and actual definitions type for #{name} differ\n"
                        pp = PP.new(error_message)
                        name.pretty_print(pp)
                        pp.breakable
                        pp.text "Expected: "
                        pp.breakable
                        pp.text expected_type.to_xml
                        pp.breakable
                        pp.text "Actual: "
                        pp.breakable
                        pp.text actual_type.to_xml
                        flunk(error_message)
                    end
                end
            end
        end
    end

    cxx_test_dir = File.expand_path('cxx_import_tests', File.dirname(__FILE__))
    generate_common_tests(cxx_test_dir)

    def cxx_test_dir
        CXXCommonTests.cxx_test_dir
    end

    def test_import_non_virtual_inheritance
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'non_virtual_inheritance.h')
        type = reg.get('/Derived')
        assert_equal ['/FirstBase', '/SecondBase'], type.metadata.get('base_classes')

        field_type = reg.get('/uint64_t')
        expected = [
            ['first', field_type, 0],
            ['second', field_type, 8],
            ['third', field_type, 16]
        ]
        assert_equal expected, type.fields.map { |name, field_type| [name, field_type, type.offset_of(name)] }
    end

    def test_import_virtual_methods
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'virtual_methods.h')
        assert !reg.include?('/Class')
    end

    def test_import_virtual_inheritance
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'virtual_inheritance.h')
        assert reg.include?('/Base')
        assert !reg.include?('/Derived')
    end

    def test_import_private_base_class
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'private_base_class.h')
        assert reg.include?('/Base')
        assert !reg.include?('/Derived')
    end

    def test_import_ignored_base_class
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'ignored_base_class.h')
        assert !reg.include?('/Base')
        assert !reg.include?('/Derived')
    end

    def test_import_template_of_container
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'template_of_container.h')
        assert reg.include?('/BaseTemplate</std/vector</double>>')
    end

    def test_import_documentation_parsing_handles_opening_bracket_and_struct_definition_on_different_lines
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'documentation_with_struct_and_opening_bracket_on_different_lines.h')
        assert_equal ["this is a multiline\ndocumentation block"], reg.get('/DocumentedType').metadata.get('doc')
    end

    def test_import_documentation_parsing_handles_spaces_between_opening_bracket_and_struct_definition
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'documentation_with_space_between_struct_and_opening_bracket.h')
        assert_equal ["this is a multiline\ndocumentation block"], reg.get('/DocumentedType').metadata.get('doc')
    end

    def test_import_documentation_parsing_handles_opening_bracket_and_struct_definition_on_the_same_line
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'documentation_with_struct_and_opening_bracket_on_the_same_line.h')
        assert_equal ["this is a multiline\ndocumentation block"], reg.get('/DocumentedType').metadata.get('doc')
    end

    def test_import_supports_utf8
        reg = Typelib::Registry.import File.join(cxx_test_dir, 'documentation_utf8.h')
        assert_equal ["this is a \u9999 multiline with \u1290 unicode characters"], reg.get('/DocumentedType').metadata.get('doc')
    end
end
