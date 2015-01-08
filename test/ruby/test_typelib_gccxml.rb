require 'typelib/test'

class TC_TypelibGCCXML < Minitest::Test
    include Typelib

    attr_reader :cxx_test_dir

    def setup
        @cxx_test_dir = File.expand_path('cxx_import_tests', File.dirname(__FILE__))
        super
    end

    def test_tokenize_template
        assert_equal ['int'],
            Typelib::GCCXMLLoader.template_tokenizer('int')
        assert_equal ['std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>'],
            Typelib::GCCXMLLoader.template_tokenizer('std::vector<int, std::allocator<int> >')

        assert_equal ['std::vector', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', ',', 'std::allocator', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', '>', '>'],
            Typelib::GCCXMLLoader.template_tokenizer('std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >')

        assert_equal ['std::vector', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', ',', 'std::allocator', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', '>', '>', '::size_t'],
            Typelib::GCCXMLLoader.template_tokenizer('std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >::size_t')
    end

    def test_collect_template_arguments
        tokenized = %w{< < bla , 6 , blo > , test > ::int>}
        assert_equal [%w{< bla , 6 , blo >}, %w{test}],
            Typelib::GCCXMLLoader.collect_template_arguments(tokenized)

    end

    def test_cxx_to_typelib_subtype_of_container
        cxx_name = 'std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >::size_t'
        assert_equal '/std/vector</std/vector</int>>/size_t',
            Typelib::GCCXMLLoader.cxx_to_typelib(cxx_name)
    end

    def test_cxx_to_typelib_template_of_container
        cxx_name = 'BaseTemplate<std::vector<double, std::allocator<double> > >'
        assert_equal '/BaseTemplate</std/vector</double>>',
            Typelib::GCCXMLLoader.cxx_to_typelib(cxx_name)
    end

    def test_parse_template
        assert_equal ['int', []],
            Typelib::GCCXMLLoader.parse_template('int')
        assert_equal ['std::vector', ['int', 'std::allocator<int>']],
            Typelib::GCCXMLLoader.parse_template('std::vector<int, std::allocator<int> >')
        assert_equal ['std::vector', ['std::vector<int,std::allocator<int>>', 'std::allocator<std::vector<int,std::allocator<int>>>']],
            Typelib::GCCXMLLoader.parse_template('std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >')

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

