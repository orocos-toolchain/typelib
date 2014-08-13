require 'typelib/test'
require 'utilrb/hash/map_value'

class TC_RubyMappingCustomization < Minitest::Test
    def test_it_should_register_basic_typenames_in_the_from_typename_set
        mapping = Typelib::RubyMappingCustomization.new
        mapping.set("/basic", obj = Object.new)
        assert_equal Hash["/basic" => [obj]], mapping.from_typename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_array_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_container_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_regexp.map_value { |k,v| v.map(&:last) }
    end
    def test_it_should_register_catchall_array_typenames_in_the_from_array_basename_set
        mapping = Typelib::RubyMappingCustomization.new
        mapping.set("/basic[]", obj = Object.new)
        assert_equal Hash[], mapping.from_typename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash["/basic" => [obj]], mapping.from_array_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_container_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_regexp.map_value { |k,v| v.map(&:last) }
    end
    def test_it_should_register_catchall_container_typenames_in_the_from_container_basename_set
        mapping = Typelib::RubyMappingCustomization.new
        mapping.set("/basic<>", obj = Object.new)
        assert_equal Hash[], mapping.from_typename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_array_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash["/basic" => [obj]], mapping.from_container_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_regexp.map_value { |k,v| v.map(&:last) }
    end
    def test_it_should_register_non_strings_in_the_from_regexp_set
        mapping = Typelib::RubyMappingCustomization.new
        key = flexmock
        mapping.set(key, obj = Object.new)
        assert_equal Hash[], mapping.from_typename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_array_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[], mapping.from_container_basename.map_value { |k,v| v.map(&:last) }
        assert_equal Hash[key => [obj]], mapping.from_regexp.map_value { |k,v| v.map(&:last) }
    end
    def test_it_should_find_basic_types_by_name_and_regexp
        registry = Typelib::CXXRegistry.new

        mapping = Typelib::RubyMappingCustomization.new
        mapping.set("/int8_t", obj1 = Object.new)
        mapping.set(/int8_t$/, obj2 = Object.new)
        assert_equal [obj1, obj2], mapping.find_all(registry.get("/int8_t"))
    end
    def test_it_should_find_array_types_by_name_element_name_and_regexp
        registry = Typelib::CXXRegistry.new

        mapping = Typelib::RubyMappingCustomization.new
        mapping.set("/int8_t[]", obj1 = Object.new)
        mapping.set("/int8_t[8]", obj2 = Object.new)
        mapping.set(/int8_t\[\d+\]$/, obj3 = Object.new)
        assert_equal [obj1, obj2, obj3], mapping.find_all(registry.build("/int8_t[8]"))
    end
    def test_it_should_find_container_types_by_name_container_name_and_regexp
        registry = Typelib::CXXRegistry.new

        mapping = Typelib::RubyMappingCustomization.new
        mapping.set("/std/vector<>", obj1 = Object.new)
        mapping.set("/std/vector</float>", obj2 = Object.new)
        mapping.set(/vector</, obj3 = Object.new)
        assert_equal [obj1, obj2, obj3], mapping.find_all(registry.create_container("/std/vector", "/float"))
    end
end

class TC_Specializations < Minitest::Test
end

