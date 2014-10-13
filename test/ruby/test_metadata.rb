require 'typelib/test'

class TC_MetaData < Minitest::Test
    attr_reader :type, :registry
    
    def setup
        @registry = Typelib::CXXRegistry.new
        @type = registry.create_compound '/Test' do |c|
            c.add 'field', '/double'
        end
        super
    end
    def test_it_supports_utf8_as_encoding
        metadata = Typelib::MetaData.new
        key = "\u2314"
        value = "\u3421"
        metadata.set(key, value)
        assert_equal [[key, [value]]], metadata.each.to_a
    end
    def test_it_can_set_values_for_a_key
        metadata = Typelib::MetaData.new
        metadata.add('k', 'v0')
        metadata.add('k', 'v1')
        metadata.set('k', 'v')
        assert_equal ['v'], metadata.get('k')
    end
    def test_it_can_add_values_to_a_key
        metadata = Typelib::MetaData.new
        metadata.add('k', 'v0')
        assert_equal ['v0'], metadata.get('k')
        metadata.add('k', 'v1')
        assert_equal ['v0', 'v1'], metadata.get('k')
    end
    def test_it_can_clear_values_for_a_key
        metadata = Typelib::MetaData.new
        metadata.set('k0', 'v0')
        metadata.set('k1', 'v1')
        metadata.clear('k1')
        assert_equal [['k0', ['v0']]], metadata.each.to_a
    end
    def test_it_can_clear_all_values
        metadata = Typelib::MetaData.new
        metadata.set('k0', 'v0')
        metadata.set('k1', 'v1')
        metadata.clear
        assert_equal [], metadata.each.to_a
    end
    def test_it_can_be_accessed_for_types
        assert_kind_of Typelib::MetaData, type.metadata
    end
    def test_it_can_be_accessed_for_fields
        assert_kind_of Typelib::MetaData, type.field_metadata['field']
    end
    def test_it_is_marshalled_and_unmarshalled_for_types
        type.metadata.set('k0', 'v0')
        new_registry = Typelib::Registry.from_xml(registry.to_xml)
        assert_equal [['k0', ['v0']]], new_registry.get('/Test').metadata.each.to_a
    end
    def test_it_is_marshalled_and_unmarshalled_for_fields
        type.field_metadata['field'].set('k0', 'v0')
        new_registry = Typelib::Registry.from_xml(registry.to_xml)
        assert_equal [['k0', ['v0']]], new_registry.get('/Test').field_metadata['field'].each.to_a
    end
end


