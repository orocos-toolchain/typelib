module CXX_GCCXML_Common
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

    def test_split_first_namespace
        assert_equal ["/NS2/", 'NS3/Test'], Typelib::GCCXMLLoader.split_first_namespace("/NS2/NS3/Test")
        assert_equal ["/Matrix</double,3,1>/", 'Scalar'], Typelib::GCCXMLLoader.split_first_namespace("/Matrix</double,3,1>/Scalar")
        assert_equal ["/Matrix</double,3,1>/", 'Gaussian</double,3>/Scalar'], Typelib::GCCXMLLoader.split_first_namespace("/Matrix</double,3,1>/Gaussian</double,3>/Scalar")
        assert_equal ["/std/", "vector</wrappers/Matrix</double,3,1>>"], Typelib::GCCXMLLoader.split_first_namespace("/std/vector</wrappers/Matrix</double,3,1>>")
        assert_equal ["/", "vector</wrappers/Matrix</double,3,1>>"], Typelib::GCCXMLLoader.split_first_namespace("/vector</wrappers/Matrix</double,3,1>>")
        assert_equal ["/nested_types/", "Outside/Inside"], Typelib::GCCXMLLoader.split_first_namespace("/nested_types/Outside/Inside")
    end

    def test_split_last_namespace
        assert_equal ["/NS2/NS3/", 'Test'], Typelib::GCCXMLLoader.split_last_namespace("/NS2/NS3/Test")
        assert_equal ["/wrappers/Matrix</double,3,1>/", 'Scalar'], Typelib::GCCXMLLoader.split_last_namespace("/wrappers/Matrix</double,3,1>/Scalar")
        assert_equal ["/wrappers/Matrix</double,3,1>/Gaussian</double,3>/", 'Scalar'], Typelib::GCCXMLLoader.split_last_namespace("/wrappers/Matrix</double,3,1>/Gaussian</double,3>/Scalar")
        assert_equal ["/std/", "vector</wrappers/Matrix</double,3,1>>"], Typelib::GCCXMLLoader.split_last_namespace("/std/vector</wrappers/Matrix</double,3,1>>")
        assert_equal ["/", "vector</wrappers/Matrix</double,3,1>>"], Typelib::GCCXMLLoader.split_last_namespace("/vector</wrappers/Matrix</double,3,1>>")
    end
end

