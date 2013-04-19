require 'test/unit'
require 'typelib'

class TC_TypelibGCCXML < Test::Unit::TestCase
    include Typelib

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

    def test_cxx_to_typelib
        cxx_name = 'std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >::size_t'
        assert_equal '/std/vector</std/vector</int>>/size_t',
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
end

