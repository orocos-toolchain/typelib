require 'typelib/test'
require_relative './cxx_common_tests'

class TC_CXX_GCCXML < Minitest::Test
    include Typelib
    include CXXCommonTests

    def setup
        super
        setup_loader 'gccxml'
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

    def test_normalize_type_name_does_not_add_a_slash_to_numeric_template_parameters
        loader = flexmock(self.loader.new, find_node_by_name: nil)
        assert_equal '/Eigen/Matrix</double,0,1,2>',
            loader.normalize_type_name('/Eigen/Matrix</double,0,1,2>')
    end
end

