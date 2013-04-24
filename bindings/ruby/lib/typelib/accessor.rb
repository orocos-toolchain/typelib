module Typelib
    # An object that gives direct access to a set of values child of a root
    class Accessor
        # The set of paths describing the required fields
        attr_reader :paths

        def initialize(paths = Array.new)
            @paths = paths
        end

        # Builds an accessor that gives access to all the fields whose type
        # matches the given block in +type_model+
        def self.find_in_type(type_model, &block)
            matches = traverse_and_find_in_type(type_model, &block)
            Accessor.new(matches || [])
        end

        def self.traverse_and_find_in_type(type_model)
            result = []

            # First, check if type_model itself is wanted
            if yield(type_model)
                result << Path.new([])
            end

            if type_model <= Typelib::CompoundType
                type_model.each_field do |field_name, field_type|
                    if matches = traverse_and_find_in_type(field_type, &proc)
                        matches.each do |path|
                            path.unshift_call(:raw_get, field_name)
                        end
                        result.concat(matches)
                    end
                end
            elsif type_model <= Typelib::ArrayType || type_model <= Typelib::ContainerType
                if matches = traverse_and_find_in_type(type_model.deference, &proc)
                    matches.each do |path|
                        path.unshift_iterate(:raw_each)
                    end
                    result.concat(matches)
                end
            end
            result
        end

        def each(root)
            if !block_given?
                return enum_for(:each, root)
            end

            paths.each do |p|
                p.resolve(root).each do |obj|
                    yield(obj)
                end
            end
            self
        end
        include Enumerable
    end
end

