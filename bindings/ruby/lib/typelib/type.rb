module Typelib
    # Base class for all types
    # Registry types are wrapped into subclasses of Type
    # or other Type-derived classes (Array, Pointer, ...)
    #
    # Value objects are wrapped into instances of these classes
    class Type
        allowed_overloadings = instance_methods
        allowed_overloadings = allowed_overloadings.map(&:to_s).to_set
        allowed_overloadings.delete_if { |n| n =~ /^__/ }
        allowed_overloadings -= ["class"]
        allowed_overloadings |= allowed_overloadings.map(&:to_sym).to_set
        ALLOWED_OVERLOADINGS = allowed_overloadings.to_set

        class << self
            # The metadata object
            attr_reader :metadata

            # Definition of the unique convertion that should be used to convert
            # this type into a Ruby object
            #
            # The value is [ruby_class, options, block]. It is saved there only
            # for convenience purposes, as it is not used by Typelib.to_ruby
            #
            # If nil, no convertions are set. ruby_class might be nil if no
            # class has been specified
            attr_accessor :convertion_to_ruby

            # Definition of the convertions between Ruby objects to this
            # Typelib type. It is used by Typelib.from_ruby.
            #
            # It is a mapping from a Ruby class K to a block which can convert a
            # value of class K to the corresponding Typelib value
            attr_reader :convertions_from_ruby

            attr_predicate :contains_converted_types?, true

            def compatible_with_memcpy?
                layout = begin memory_layout
                         rescue
                             return false # no layout for this type
                         end
                layout.size == 2 &&
                    ((layout[0] == :FLAG_MEMCPY) || layout[0] == :FLAG_SKIP)
            end

            def filter_methods_that_should_not_be_defined(names, with_raw, &block)
                Typelib.filter_methods_that_should_not_be_defined(self, self, names, Type::ALLOWED_OVERLOADINGS, nil, with_raw, &block)
            end

            def define_method_if_possible(name, &block)
                Typelib.define_method_if_possible(self, self, name, Type::ALLOWED_OVERLOADINGS, &block)
            end
        end
        @convertions_from_ruby = Hash.new

        # True if this type refers to subtype of the given type, or if it a
        # subtype of +type+ itself
        def self.contains?(type)
            self <= type ||
                recursive_dependencies.include?(type) || recursive_dependencies.any? { |t| t <= type }
        end

        # @deprecated
        #
        # Replaced by {recursive_dependencies}
        def self.dependencies
            direct_dependencies
        end

        # [ValueSet<Type>] Returns the types that are directly referenced by +self+
        #
        # @see recursive_dependencies
        def self.direct_dependencies
            @direct_dependencies ||= do_dependencies
        end

        # Returns the set of all types that are needed to define +self+
        #
        # @param [ValueSet<Type>] set if given, the new types will be added to
        #   this set. Otherwise, a new set is created. In both cases, the set is
        #   returned
        # @return [ValueSet<Type>]
        def self.recursive_dependencies(set = nil)
            if !@recursive_dependencies
                @recursive_dependencies = ValueSet.new
                direct_dependencies.each do |direct_dep|
                    @recursive_dependencies << direct_dep
                    direct_dep.recursive_dependencies(@recursive_dependencies)
                end
                @recursive_dependencies
            end

            if set then return set.merge(@recursive_dependencies)
            else return @recursive_dependencies
            end
        end

        # Extends this type class to have values of this type converted by the
        # given block on C++/Ruby bondary
        def self.convert_to_ruby(to = nil, options = Hash.new, &block)
            options = Kernel.validate_options options,
                :recursive => true

            block = lambda(&block)
            m = Module.new do
		define_method(:to_ruby, &block)
            end
            extend(m)

            options[:block] = block

            self.contains_converted_types = options[:recursive]
            self.convertion_to_ruby = [to, options]
        end

        # Extends this type class to have be able to use the Ruby class +from+
        # to initialize a value of type +self+
        def self.convert_from_ruby(from, &block)
            convertions_from_ruby[from] = lambda(&block)
        end

        # Called by Typelib when a subclass is created.
        def self.subclass_initialize
            Typelib.type_specializations.find_all(self).each do |m|
                extend m
            end
            Typelib.value_specializations.find_all(self).each do |m|
                include m
            end

            @convertions_from_ruby = Hash.new
            Typelib.convertions_from_ruby.find_all(self).each do |conv|
                convert_from_ruby(conv.ruby, &conv.block)
            end
            if conv = Typelib.convertions_to_ruby.find(self, false)
                convert_to_ruby(conv.ruby, &conv.block)
            end

            if respond_to?(:extend_for_custom_convertions)
                extend_for_custom_convertions
            end

            super if defined? super
        end

        # Returns a new Type instance that contains the same value, but using a
        # different type object
        #
        # It raises ArgumentError if the cast is invalid.
        #
        # The ability to cast can be checked beforehand by using Type.casts_to?
        #
        # Note that the return value might be +self+, and that both objects
        # refer to the same memory zone. Therefore, if one of the two value
        # objects is used to modify the underlying value, that will be reflected
        # in the other. Moreover, both values should not be modified in two
        # different threads without proper locking.
        def cast(target_type)
            if !self.class.casts_to?(target_type)
                raise ArgumentError, "cannot cast #{self} to #{target_type}"
            end
            do_cast(target_type)
        end

        # Returns the value that is parent of this one. Root values return nil
        def __typelib_parent
            @parent
        end

        # Called internally to apply any change from a converted value to the
        # underlying Typelib value
        def apply_changes_from_converted_types
        end

        # Called internally to tell typelib that converted values should be
        # updated next time from the underlying Typelib value
        # underlying Typelib value
        def invalidate_changes_from_converted_types
        end

        # Creates a deep copy of this value.
        #
        # It is guaranteed that this value will be referring to a different
        # memory zone than +self+
	def dup
	    new = self.class.new
	    Typelib.copy(new, self)
	end
        alias clone dup

        module Invalidate
            def to_memory_ptr; raise TypeError, "invalidated object" end
            def to_ruby; raise TypeError, "invalidated object" end
            def to_byte_array; raise TypeError, "invalidated object" end
            def marshalling_size; raise TypeError, "invalidated object" end
        end

        # Call to freeze the object, i.e. to make it readonly
        def freeze
            freeze_children
            @__typelib_frozen = true
            self
        end

        def frozen?
            @__typelib_frozen
        end

        # Call to forbid any R/W access to the underlying memory zone. This is
        # usually used by the underlying memory management in cases where the
        # memory zone itself got deallocated
        def invalidate
            do_invalidate
            invalidate_children
            @parent = nil
            @__typelib_invalidated = true
            extend Invalidate
            self
        end

        # True if this object has been invalidated by calling #invalidate
        def invalidated?
            @__typelib_invalidated
        end

        # Method that is used to track some important things when doing an
        # operation (from C++) that can possibly change the memory allocated for
        # this object
        def allocating_operation
            current_size = marshalling_size
            handle_invalidation do
                yield
            end
        ensure
            if current_size
                Typelib.add_allocated_memory([marshalling_size - current_size, 0].max)
            end
        end

        # Computes the paths that will allow to enumerate all containers
        # contained in values of this type
        #
        # It is mostly useful in Type#handle_invalidation
        # 
        # @return [Accessor]
        def self.containers_accessor
            @containers ||= Accessor.find_in_type(self.class) do |t|
                t <= Typelib::ContainerType
            end
        end

        # Handles invalidation of containers due to the provided block
        def handle_invalidation(&block)
            # We now need to recursively find all containers in +from+, and make
            # sure they get proper invalidation support
            accessor = self.class.containers_accessor
            containers = accessor.each(self).to_a
            handle_invalidation_protect_containers(containers, &block)
        end

        # Helper method for #handle_invalidation.
        def handle_invalidation_protect_containers(containers, &block)
            if containers.empty?
                yield
            else
                v = containers.pop
                v.handle_container_invalidation do
                    handle_container_invalidation(containers, &block)
                end
            end
        end

        class << self
	    # The Typelib::Registry this type belongs to
            attr_reader :registry

	    # The type's full name (i.e. name and namespace). In typelib,
	    # namespace components are separated by '/'
	    def name
		if defined? @name
		    @name
		else
		    super
		end
	    end

            # Helper class that validates the options given to
            # Type.memory_layout and Type#to_byte_array
            def validate_layout_options(options)
                Kernel.validate_options options, :accept_opaques => false,
                    :accept_pointers => false,
                    :merge_skip_copy => true,
                    :remove_trailing_skips => true
            end

            # Returns a representation of the MemoryLayout for this type.
            #
            # The generated layout can be changed by setting one or more
            # following options:
            #
            # accept_opaques::
            #   accept types with opaques. Fields/values that are opaques are
            #   simply skipped. This is false by default: types with opaques are
            #   generating an error.
            # accept_pointers::
            #   accept types with pointers. Fields/values that are pointer are
            #   simply skipped. This is false by default: types with pointers
            #   are generating an error.
            # merge_skip_copy::
            #   in a layout, zones that contain data are copied, while zones
            #   that are there because of C++ padding rules are skipped. If this
            #   is true (the default), consecutive copy/skips are merged into
            #   one bigger copy, as doine one big memcpy() is probably more
            #   efficient than skipping the few padding bytes. Set to false to
            #   turn that off.
            # remove_trailing_skips::
            #   because of C/C++ padding rules, structures might contain
            #   trailing bytes that don't contain information. If this option is
            #   true (the default), these bytes are removed from the layout.
            def memory_layout(options = Hash.new)
                options = validate_layout_options(options)
                do_memory_layout(
                    options[:accept_pointers],
                    options[:accept_opaques],
                    options[:merge_skip_copy],
                    options[:remove_trailing_skips])
            end

	    # Returns the namespace part of the type's name.  If +separator+ is
	    # given, the namespace components are separated by it, otherwise,
	    # the default of Typelib::NAMESPACE_SEPARATOR is used. If nil is
	    # used as new separator, no change is made either.
	    def namespace(separator = Typelib::NAMESPACE_SEPARATOR, remove_leading = false)
                Typelib.namespace(name, separator, remove_leading)
	    end

            # Returns the basename part of the type's name, i.e. the type name
            # without the namespace part.
            #
            # See also Typelib.basename
            def basename(separator = Typelib::NAMESPACE_SEPARATOR)
                Typelib.basename(name, separator)
            end

            # Returns the complete name for the type (both namespace and
            # basename). If +separator+ is set to a value different than
            # Typelib::NAMESPACE_SEPARATOR, Typelib's namespace separator will
            # be replaced by the one given in argument.
            #
            # For instance,
            #
            #   type_t.full_name('::')
            #
            # will return the C++ name for the given type
	    def full_name(separator = Typelib::NAMESPACE_SEPARATOR, remove_leading = false)
		result = namespace(separator, remove_leading) + basename(separator)
	    end

	    def to_s; "#<#{superclass.name}: #{name}>" end

	    # are we a null type ?
	    def null?; @null end
            # True if this type is opaque
            #
            # Values from opaque types cannot be manipulated by Typelib. They
            # are usually used to refer to fields that will be converted first
            # (by some unspecified means) to a value that Typelib can manipulate
            def opaque?; @opaque end

	    # Returns the pointer-to-self type
            def to_ptr; registry.build(name + "*") end

            # Given a markdown-formatted string, return what should be displayed
            # as text
            def pp_doc(pp, doc)
                if !doc.empty?
                    first_line = true
                    doc = doc.split("\n").map do |line|
                        if first_line
                            first_line = false
                            "/** " + line
                        else " * " + line
                        end
                    end
                    if doc.size == 1
                        doc[0] << " */"
                    else
                        doc << " */"
                    end

                    first_line = true
                    doc.each do |line|
                        if !first_line
                            pp.breakable
                        end
                        pp.text line
                        first_line = false
                    end
                    true
                end
            end

            def pretty_print(pp, with_doc = true) # :nodoc:
                if with_doc && (doc = metadata.get('doc').first)
                    if pp_doc(pp, doc)
                        pp.breakable
                    end
                end
		pp.text name 
	    end

            alias :__real_new__ :new

            # Creates a new value from either a MemoryZone instance (i.e. a
            # pointer), or from a marshalled version of a value.
            #
            # This is usually used to reload a value marshalled with
            # to_byte_array
            #
            # For instance,
            #
            #   type = registry.get 'A'
            #   value = type.new
            #   # modify +value+
            #   marshalled = value.to_byte_array
            #   # save +marshalled+ to a file (for instance)
            #
            # Later on ...
            #
            #   # load +marshalled+ from the file
            #   type = registry.get 'A'
            #   value = type.wrap(marshalled)
            #
            def wrap(ptr)
		if null?
		    raise TypeError, "this is a null type"
                elsif !(ptr.kind_of?(String) || ptr.kind_of?(MemoryZone))
                    raise ArgumentError, "can only wrap strings and memory zones"
		end
                new_value = __real_new__(ptr) 
                if size = new_value.marshalling_size
                    Typelib.add_allocated_memory(size)
                end
                new_value
	    end

            # Creates a new value of the given type.
            #
            # Note that the value is *not* initialized. To initialize a value to
            # zero, one can call Type#zero!
            def new(init = nil)
                if init
                    Typelib.from_ruby(init, self)
                else
                    __real_new__(nil)
                end
            end

	    # Check if this type is a +typename+. If +typename+
	    # is a string or a regexp, we match it against the type
	    # name. Otherwise we call Class#<
	    def is_a?(typename)
		if typename.respond_to?(:to_str)
		    typename.to_str === self.name
		elsif Regexp === typename
		    typename === self.name
		else
		    self <= typename
		end
	    end
        end

        # Returns a string whose content is a marshalled representation of the memory
        # hold by +obj+
        #
        # This can be used to create a new object later by using value_type.wrap, where
        # +value_type+ is the object returned by Registry#get. Example:
        #
        #   # Do complex computation
        #   marshalled_data = result.to_byte_array
        #
        #   # Later on ...
        #   value = my_registry.get('/base/Type').wrap(marshalled_data)
        def to_byte_array(options = Hash.new)
            options = Type.validate_layout_options(options)
            do_byte_array(
                options[:accept_pointers],
                options[:accept_opaques],
                options[:merge_skip_copy],
                options[:remove_trailing_skips])
        end


	def initialize(*args)
	    __initialize__(*args)
	end

        def freeze_children
        end

        def invalidate_children
        end

        def to_ruby
            Typelib.to_ruby(self, self.class)
        end

	# Check for value equality
        def ==(other)
	    # If other is also a type object, we first
	    # check basic constraints before trying conversion
	    # to Ruby objects
            if Type === other
		return Typelib.compare(self, other)
	    else
                # +other+ is a Ruby type. Try converting +self+ to ruby and
                # check for equality in Ruby objects
		if (ruby_value = Typelib.to_ruby(self)).eql?(self)
		    return false
		end
		other == ruby_value
	    end
        end

	# Returns a PointerType object which points to +self+. Note that
	# to_ptr.deference == self
        def to_ptr
            pointer = self.class.to_ptr.wrap(@ptr.to_ptr)
	    pointer.instance_variable_set(:@points_to, self)
	    pointer
        end
	
	def to_s # :nodoc:
	    if respond_to?(:to_str)
		to_str
	    elsif ! (ruby_value = to_ruby).eql?(self)
		ruby_value.to_s
	    else
		"#<#{self.class.name}: 0x#{address.to_s(16)} ptr=0x#{@ptr.zone_address.to_s(16)}>"
	    end
	end

	def pretty_print(pp) # :nodoc:
	    pp.text to_s
	end

        # Get the memory pointer for self
        def to_memory_ptr; @ptr end

	def is_a?(typename); self.class.is_a?(typename) end

        def inspect
            sprintf("#<%s:%s @%s>", self.class.superclass.name, self.class.name, to_memory_ptr.inspect)
        end
    end
end
