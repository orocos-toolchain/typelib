require 'typelib/test'
require_relative './cxx_common_tests'

class TC_CXX_GCCXML < Minitest::Test
    include Typelib
    include CXXCommonTests

    def setup
        super
        setup_loader 'gccxml', castxml: false
    end

    def test_cxx_to_typelib_subtype_of_container
        cxx_name = 'std::vector <std::vector <int ,std::allocator<int>>, std::allocator< std::vector<int , std::allocator <int>>      > >::size_t'
        assert_equal '/std/vector</std/vector</int,/std/allocator</int>>,/std/allocator</std/vector</int,/std/allocator</int>>>>/size_t',
            Typelib::GCCXMLLoader.cxx_to_typelib(cxx_name)
    end

    def test_cxx_to_typelib_template_of_container
        cxx_name = 'BaseTemplate<std::vector<double, std::allocator<double> > >'
        assert_equal '/BaseTemplate</std/vector</double,/std/allocator</double>>>',
            Typelib::GCCXMLLoader.cxx_to_typelib(cxx_name)
    end
end

