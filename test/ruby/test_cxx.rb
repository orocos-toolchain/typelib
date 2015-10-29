require 'typelib/test'

class TC_CXX < Minitest::Test
    def test_template_tokenizer
        assert_equal ['int'],
            Typelib::CXX.template_tokenizer('int')
        assert_equal ['std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>'],
            Typelib::CXX.template_tokenizer('std::vector<int, std::allocator<int> >')

        assert_equal ['std::vector', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', ',', 'std::allocator', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', '>', '>'],
            Typelib::CXX.template_tokenizer('std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >')

        assert_equal ['std::vector', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', ',', 'std::allocator', '<', 'std::vector', '<', 'int', ',', 'std::allocator', '<', 'int', '>', '>', '>', '>', '::size_t'],
            Typelib::CXX.template_tokenizer('std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >::size_t')
    end

    def test_collect_template_arguments
        tokenized = %w{< < bla , 6 , blo > , test > ::int>}
        assert_equal [%w{< bla , 6 , blo >}, %w{test}],
            Typelib::GCCXMLLoader.collect_template_arguments(tokenized)
    end

    def test_parse_template
        assert_equal ['int', []],
            Typelib::CXX.parse_template('int')
        assert_equal ['std::vector', ['int', 'std::allocator<int>']],
            Typelib::CXX.parse_template('std::vector<int, std::allocator<int> >')
        assert_equal ['std::vector', ['std::vector<int,std::allocator<int>>', 'std::allocator<std::vector<int,std::allocator<int>>>']],
            Typelib::CXX.parse_template('std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >')

    end
end
