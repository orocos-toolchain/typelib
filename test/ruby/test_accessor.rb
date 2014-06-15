require 'typelib/test'

class TC_TypeAccessor < Minitest::Test
    attr_reader :type, :registry
    def setup
        @registry = Typelib::CXXRegistry.new
        registry.create_container '/std/vector', '/float'
        registry.create_compound '/Intern' do |c|
            c.vector = "/std/vector</float>"
            c.array = "/float[10]"
            c.direct = "/float"
        end
        registry.create_container '/std/vector', '/Intern'
        registry.create_compound '/Root' do |c|
            c.vector = "/std/vector</Intern>"
            c.array = "/Intern[10]"
            c.direct = "/Intern"
            c.fl_vector = "/std/vector</float>"
            c.fl_array = "/float[10]"
            c.fl_direct = "/float"
        end
        @type = registry.get '/Root'
    end

    def test_it_resolves_all_accessible_paths
        accessor = Typelib::Accessor.find_in_type type do |t|
            t.name == "/float"
        end
        assert_equal 12, accessor.paths.size
        path_set = accessor.paths.map { |p| p.elements.dup }

        assert path_set.delete([[:call, [:raw_get, 'fl_direct']]])
        assert path_set.delete([[:call, [:raw_get, 'fl_array']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'fl_vector']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'direct']],
                                [:call, [:raw_get, 'direct']]])
        assert path_set.delete([[:call, [:raw_get, 'direct']],
                                [:call, [:raw_get, 'array']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'direct']],
                                [:call, [:raw_get, 'vector']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'array']],
                                [:iterate, [:raw_each]],
                                [:call, [:raw_get, 'direct']]])
        assert path_set.delete([[:call, [:raw_get, 'array']],
                                [:iterate, [:raw_each]],
                                [:call, [:raw_get, 'array']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'array']],
                                [:iterate, [:raw_each]],
                                [:call, [:raw_get, 'vector']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'vector']],
                                [:iterate, [:raw_each]],
                                [:call, [:raw_get, 'direct']]])
        assert path_set.delete([[:call, [:raw_get, 'vector']],
                                [:iterate, [:raw_each]],
                                [:call, [:raw_get, 'array']],
                                [:iterate, [:raw_each]]])
        assert path_set.delete([[:call, [:raw_get, 'vector']],
                                [:iterate, [:raw_each]],
                                [:call, [:raw_get, 'vector']],
                                [:iterate, [:raw_each]]])
        assert path_set.empty?
    end

    def test_it_resolves_matching_containers_recursively
        type = registry.create_container '/std/vector', '/std/vector</float>'
        accessor = Typelib::Accessor.find_in_type(type) { |t| t <= Typelib::ContainerType }

        path_set = accessor.paths.map { |p| p.elements.dup }
        assert_equal 2, path_set.size

        assert path_set.include?([])
        assert path_set.include?([[:iterate, [:raw_each]]])
    end
end

