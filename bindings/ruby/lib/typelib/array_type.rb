module Typelib
    # Base class for static-length arrays
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class ArrayType < IndirectType
        attr_reader :element_t

        # Module used to extend frozen values
        module Freezer
            def do_set(*args)
                raise TypeError, "frozen object"
            end
        end

        # Module used to extend invalidated values
        module Invalidate
            def do_set(*args); raise TypeError, "invalidated object" end
            def do_each(*args); raise TypeError, "invalidated object" end
            def do_get(*args); raise TypeError, "invalidated object" end
            def size; raise TypeError, "invalidated object" end
        end

        # Call to freeze the object, i.e. to make it readonly
        def freeze
            super
            # Nothing to do here
            extend Freezer
            self
        end

        # Call to invalidate the object, i.e. to forbid any R/W access to it.
        # This is used in the framework when the underlying memory zone has been
        # made invalid by some operation (as e.g. container resizing)
        def invalidate
            super
            extend Invalidate
            self
        end

        def typelib_initialize
            super
            @element_t = self.class.deference
            @elements = Array.new
        end

        def self.find_custom_convertions(conversion_set)
            generic_array_id = deference.name + '[]'
            super(conversion_set) +
                super(conversion_set, generic_array_id)
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

	def pretty_print(pp) # :nodoc:
            apply_changes_from_converted_types
	    all_fields = enum_for(:each_with_index).to_a

	    pp.text '['
	    pp.nest(2) do
		pp.breakable
		pp.seplist(all_fields) do |element|
		    element, index = *element 
		    pp.text "[#{index}] = "
		    element.pretty_print(pp)
		end
	    end
	    pp.breakable
	    pp.text ']'
	end

        def raw_each
            if !block_given?
                return enum_for(:raw_each)
            end

            self.class.length.times do |i|
                yield(raw_get(i))
            end
        end

        def each
            if !block_given?
                return enum_for(:each)
            end
            raw_each do |el|
                yield(Typelib.to_ruby(el, element_t))
            end
        end

        def self.subclass_initialize
            super if defined? super

            convert_from_ruby Array do |value, expected_type|
                if value.size != expected_type.length
                    raise ArgumentError, "expected an array of size #{expected_type.length}, got #{value.size}"
                end

                t = expected_type.new
                value.each_with_index do |el, i|
                    t[i] = el
                end
                t
            end
        end

        def self.extend_for_custom_convertions
            if deference.contains_converted_types?
                self.contains_converted_types = true

                # There is a custom convertion on the elements of this array. We
                # have to convert to a Ruby array once and for all
                #
                # This can be *very* costly for big arrays
                #
                # Note that it is overriden by convertions that are explicitely
                # defined for this type (i.e. that reference this type by name)
                convert_to_ruby Array do |value|
                    # Convertion is done by value#each directly
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

        def raw_get_cached(index)
            @elements[index]
        end

        def raw_each_cached(&block)
            @elements.compact.each(&block)
        end

        def raw_get(index)
            @elements[index] ||= do_get(index)
        end

        def raw_set(index, value)
            if value.kind_of?(Type)
                attribute = raw_get(index)
                # If +value+ is already a typelib value, just do a plain copy
                if attribute.kind_of?(Typelib::Type)
                    return Typelib.copy(attribute, value)
                end
            end
            do_set(index, value)
        end

        def [](index, range = nil)
            if range
                result = []
                range.times do |i|
                    result << Typelib.to_ruby(raw_get(i + index), element_t)
                end
                result
            else
                Typelib.to_ruby(raw_get(index), element_t)
            end
        end

        def []=(index, value)
            raw_set(index, Typelib.from_ruby(value, element_t))
        end

	# Returns the pointed-to type (defined for consistency reasons)
	def self.[](index); deference end

        # Returns the description of a type using only simple ruby objects
        # (Hash, Array, Numeric and String).
        # 
        #    { 'name' => TypeName,
        #      'class' => NameOfTypeClass, # CompoundType, ...
        #      'length' => LengthOfArrayInElements,
        #      # The content of 'element' is controlled by the :recursive option
        #      'element' => DescriptionOfArrayElement,
        #      # Only if :layout_info is true
        #      'size' => SizeOfTypeInBytes 
        #    }
        #
        # @option (see Type#to_h)
        # @return (see Type#to_h)
        def self.to_h(options = Hash.new)
            info = super
            info[:length] = length
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
        # Array types are returned as either an array of their converted
        # elements, or the hash described for the :pack_simple_arrays option.
        def to_simple_value(options = Hash.new)
            if options[:pack_simple_arrays] && element_t.respond_to?(:pack_code)
                Hash[pack_code: element_t.pack_code,
                     size: self.class.length,
                     data: Base64.strict_encode64(to_byte_array)]
            else
                raw_each.map { |v| v.to_simple_value(options) }
            end
        end

        include Enumerable
    end
end
