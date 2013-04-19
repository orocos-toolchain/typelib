module Typelib
    # Base class for all dynamic containers
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class ContainerType < IndirectType
        include Enumerable

        attr_reader :element_t

        def initialize(*args)
            super
            @element_t = self.class.deference
            @elements = []
        end

        # Module used to extend frozen values
        module Freezer
            def clear; raise TypeError, "frozen object" end
            def do_set(*args); raise TypeError, "frozen object" end
            def do_delete_if(*args); raise TypeError, "frozen object" end
            def do_erase(*args); raise TypeError, "frozen object" end
            def do_push(*args); raise TypeError, "frozen object" end
        end

        # Module used to extend invalidated values
        module Invalidate
            def clear; raise TypeError, "invalidated object" end
            def size; raise TypeError, "invalidated object" end
            def length; raise TypeError, "invalidated object" end
            def do_set(*args); raise TypeError, "invalidated object" end
            def do_delete_if(*args); raise TypeError, "invalidated object" end
            def do_erase(*args); raise TypeError, "invalidated object" end
            def do_push(*args); raise TypeError, "invalidated object" end
            def do_each(*args); raise TypeError, "invalidated object" end
            def do_get(*args); raise TypeError, "invalidated object" end
        end

        # Call to freeze the object, i.e. to make it readonly
        def freeze
            super
            # Nothing to do here
            extend Freezer
            self
        end

        # Remove all elements from this container
        def clear
            do_clear
            invalidate_children
        end

        # Call to invalidate the object, i.e. to forbid any R/W access to it.
        # This is used in the framework when the underlying memory zone has been
        # made invalid by some operation (as e.g. container resizing)
        def invalidate
            super
            extend Invalidate
            self
        end

        def freeze_children
            super
            @elements.compact.each do |obj|
                obj.freeze
            end
        end

        def invalidate_children
            super
            @elements.compact.each do |obj|
                obj.invalidate
            end
            @elements.clear
        end

        # Module included in container types that offer random access
        # functionality
        module RandomAccessContainer
            # Private version of the getters, to bypass the index boundary
            # checks
            def raw_get_no_boundary_check(index)
                if ret = @elements[index]
                    return ret
                else
                    value = do_get(index)
                    if value.kind_of?(Typelib::Type)
                        @elements[index] = value
                    end
                    value
                end
            end

            # Private version of the getters, to bypass the index boundary
            # checks
            def get_no_boundary_check(index)
                raw = raw_get_no_boundary_check(index)
                Typelib.to_ruby(raw, element_t)
            end

            def raw_get(index)
                if index < 0
                    raise ArgumentError, "index out of bounds (#{index} < 0)"
                elsif index >= size
                    raise ArgumentError, "index out of bounds (#{index} >= #{size})"
                end
                raw_get_no_boundary_check(index)
            end

            # Returns the value at the given index
            def [](index, chunk_size = nil)
                if chunk_size
                    if index < 0
                        raise ArgumentError, "index out of bounds (#{index} < 0)"
                    elsif (index + chunk_size) > size
                        raise ArgumentError, "index out of bounds (#{index} + #{chunk_size} >= #{size})"
                    end
                    result = self.class.new
                    chunk_size.times do |i|
                        result.push(raw_get_no_boundary_check(index + i))
                    end
                    result
                else
                    if index < 0
                        raise ArgumentError, "index out of bounds (#{index} < 0)"
                    elsif index >= size
                        raise ArgumentError, "index out of bounds (#{index} >= #{size})"
                    end

                    get_no_boundary_check(index)
                end
            end

            def raw_set(index, value)
                if index < 0 || index >= size
                    raise ArgumentError, "index out of bounds"
                end

                do_set(index, value)
            end

            def []=(index, value)
                v = Typelib.from_ruby(value, element_t)
                raw_set(index, v)
            end
        end

        def self.subclass_initialize
            super if defined? super

            if random_access?
                include RandomAccessContainer
            end

            convert_from_ruby Array do |value, expected_type|
                t = expected_type.new
                t.concat(value)
                t
            end
        end

        def self.extend_for_custom_convertions
            if deference.contains_converted_types?
                self.contains_converted_types = true

                # There is a custom convertion on the elements of this
                # container. We have to convert to a Ruby array once and for all
                #
                # This can be *very* costly for big containers
                #
                # Note that it is called before super() so that it gets
                # overriden by convertions that are explicitely defined for this
                # type (i.e. that reference this type by name)
                convert_to_ruby Array do |value|
                    # Convertion is done by #map
                    converted = value.map { |v| v }
                    def converted.dup
                        map(&:dup)
                    end
                    converted
                end
            end

            # This is done last so that convertions to ruby that refer to this
            # type by name can override the default convertion above
            super if defined? super
        end

        # DEPRECATED. Use #push instead
        def insert(value) # :nodoc:
            Typelib.warn "Typelib::ContainerType#insert(value) is deprecated, use #push(value) instead"
            push(value)
        end

        # Adds a new value at the end of the sequence
        def push(*values)
            concat(values)
        end

        def concat(array)
            # We need to apply changes to the converted types, as the underlying
            # container might choose to reallocate the memory somewhere else
            apply_changes_from_converted_types
            for v in array
                do_push(Typelib.from_ruby(v, element_t))
            end
            invalidate_children
        end

        def raw_each
            if !block_given?
                return enum_for(:raw_each)
            end

            do_each do |el|
                yield(el)
            end
        end

        # Enumerates the elements of this container
        def each
            if !block_given?
                return enum_for(:each)
            end

            do_each do |el|
                yield(Typelib.to_ruby(el, element_t))
            end
        end

        # Erases an element from this container
        def erase(el)
            el = Typelib.from_ruby(el, element_t)
            do_erase(el)
            invalidate_children
        end

        # Deletes the elements
        def delete_if
            do_delete_if do |el|
                yield(Typelib.to_ruby(el, element_t))
            end
            invalidate_children
        end

        # True if this container is empty
        def empty?; length == 0 end

        # Appends a new element to this container
        def <<(value); push(value) end

        def pretty_print(pp)
            apply_changes_from_converted_types
            index = 0
	    pp.text '['
	    pp.nest(2) do
		pp.breakable
		pp.seplist(enum_for(:each)) do |element|
		    pp.text "[#{index}] = "
		    element.pretty_print(pp)
                    index += 1
		end
	    end
	    pp.breakable
	    pp.text ']'
        end
    end
end
