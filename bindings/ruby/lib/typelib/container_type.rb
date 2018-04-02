module Typelib
    # Base class for all dynamic containers
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class ContainerType < IndirectType
        include Enumerable

        attr_reader :element_t

        def typelib_initialize
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
            def contained_memory_id; raise TypeError, "invalidated object" end
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
            for obj in @elements
                obj.freeze if obj
            end
        end

        def invalidate_children
            super
            for obj in @elements
                obj.invalidate if obj
            end
            @elements.clear
        end

        # Module included in container types that offer random access
        # functionality
        module RandomAccessContainer
            # Private version of the getters, to bypass the index boundary
            # checks
            def raw_get_no_boundary_check(index)
                @elements[index] ||= do_get(index, true)
            end

            # Private version of the getters, to bypass the index boundary
            # checks
            def get_no_boundary_check(index)
                if @elements[index]
                    return __element_to_ruby(@elements[index])
                else
                    value = do_get(index, false)
                    if value.kind_of?(Typelib::Type)
                        @elements[index] = value
                        __element_to_ruby(value)
                    else value
                    end
                end
            end

            def raw_get(index)
                if index < 0
                    raise ArgumentError, "index out of bounds (#{index} < 0)"
                elsif index >= size
                    raise ArgumentError, "index out of bounds (#{index} >= #{size})"
                end
                raw_get_no_boundary_check(index)
            end

            def raw_get_cached(index)
                @elements[index]
            end

            def get(index)
                self[index]
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

            def set(index, value)
                self[index] = value
            end

            def []=(index, value)
                v = __element_from_ruby(value)
                raw_set(index, v)
            end

            def raw_each
                return enum_for(:raw_each) if !block_given?
                for idx in 0...size
                    yield(raw_get_no_boundary_check(idx))
                end
            end

            def each
                return enum_for(:each) if !block_given?
                for idx in 0...size
                    yield(get_no_boundary_check(idx))
                end
            end
        end

        class DeepCopyArray < Array
            def dup
                result = DeepCopyArray.new
                for v in self
                    result << v
                end
                result
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
            convert_from_ruby DeepCopyArray do |value, expected_type|
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
                    result = DeepCopyArray.new
                    for v in value
                        result << v
                    end
                    result
                end
            end

            # This is done last so that convertions to ruby that refer to this
            # type by name can override the default convertion above
            super if defined? super

            if deference.needs_convertion_to_ruby?
                include ConvertToRuby
            end
            if deference.needs_convertion_from_ruby?
                include ConvertFromRuby
            end
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

        # Create a new container with the given size and element
        def self.of_size(size, element = deference.new)
            element = Typelib.from_ruby(element, deference).to_byte_array
            from_buffer([size].pack("Q") + element * size)
        end

        # This should return an object that allows to identify whether the
        # Typelib instances pointing to elements should be invalidated after
        # certain operations
        #
        # The default always returns nil, which means that no invalidation is
        # performed
        def contained_memory_id
        end

        def handle_container_invalidation
            memory_id    = self.contained_memory_id
            yield
        ensure
            if invalidated?
                # All children have been invalidated already by #invalidate
            elsif memory_id && (memory_id != self.contained_memory_id)
                Typelib.debug { "invalidating all elements in #{raw_to_s}" }
                invalidate_children
            elsif @elements.size > self.size
                Typelib.debug { "invalidating #{@elements.size - self.size} trailing elements in #{raw_to_s}" }
                while @elements.size > self.size
                    if el = @elements.pop
                        el.invalidate
                    end
                end
            end
        end

        module ConvertToRuby
            def __element_to_ruby(value)
                Typelib.to_ruby(value, element_t)
            end
        end
        module ConvertFromRuby
            def __element_from_ruby(value)
                Typelib.from_ruby(value, element_t)
            end
        end

        def __element_to_ruby(value)
            value
        end

        def __element_from_ruby(value)
            value
        end

        def concat(array)
            # NOTE: no need to care about convertions to ruby here, as -- when
            # the elements require a convertion to ruby -- we convert the whole
            # container to Array
            allocating_operation do
                for v in array
                    do_push(__element_from_ruby(v))
                end
            end
            self
        end

        def raw_each_cached(&block)
            @elements.compact.each(&block)
        end

        def raw_each
            return enum_for(:raw_each) if !block_given?

            idx = 0
            do_each(true) do |el|
                yield(@elements[idx] ||= el)
                idx += 1
            end
        end

        # Enumerates the elements of this container
        def each
            return enum_for(:each) if !block_given?

            idx = 0
            do_each(false) do |el|
                if el.kind_of?(Typelib::Type)
                    el = (@elements[idx] ||= el)
                    el = __element_to_ruby(el)
                end

                yield(el)
                idx += 1
            end
        end

        # Erases an element from this container
        def erase(el)
            # NOTE: no need to care about convertions to ruby here, as -- when
            # the elements require a convertion to ruby -- we convert the whole
            # container to Array
            handle_invalidation do
                do_erase(__element_from_ruby(el))
            end
        end

        # Deletes the elements
        def delete_if
            # NOTE: no need to care about convertions to ruby here, as -- when
            # the elements require a convertion to ruby -- we convert the whole
            # container to Array
            handle_invalidation do
                do_delete_if do |el|
                    yield(__element_to_ruby(el))
                end
            end
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

        # Returns the description of a type using only simple ruby objects
        # (Hash, Array, Numeric and String).
        #
        #    { name: TypeName,
        #      class: NameOfTypeClass, # CompoundType, ...
        #       # The content of 'element' is controlled by the :recursive option
        #      element: DescriptionOfArrayElement,
        #      size: SizeOfTypeInBytes # Only if :layout_info is true
        #    }
        #
        # @option (see Type#to_h)
        # @return (see Type#to_h)
        def self.to_h(options = Hash.new)
            info = super
            info[:element] =
                if options[:recursive]
                    deference.to_h(options)
                else
                    deference.to_h_minimal(options)
                end
            info
        end

        # (see Type#to_simple_value)
        #
        # Container types are returned as either an array of their converted
        # elements, or the hash described for the :pack_simple_arrays option. In
        # the latter case, a 'size' field is added with the number of elements
        # in the container to allow for validation on the receiving end.
        def to_simple_value(options = Hash.new)
            apply_changes_from_converted_types
            if options[:pack_simple_arrays] && element_t.respond_to?(:pack_code)
                Hash[pack_code: element_t.pack_code,
                     size: size,
                     data: Base64.strict_encode64(to_byte_array[8..-1])]
            else
                raw_each.map { |v| v.to_simple_value(options) }
            end
        end
    end
end
