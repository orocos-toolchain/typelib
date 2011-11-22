require 'enumerator'
require 'utilrb/object/singleton_class'
require 'utilrb/kernel/options'
require 'utilrb/module/attr_predicate'
require 'utilrb/module/const_defined_here_p'
require 'delegate'
require 'pp'
require 'facets/string/camelcase'
require 'set'

if !defined?(Infinity)
    Infinity = 1e200 ** 200
end
if !defined?(Inf)
    Inf = Infinity
end
if !defined?(NaN)
    NaN = Infinity / Infinity
end

# Typelib is the main module for Ruby-side Typelib functionality.
#
# Typelib allows to do two things:
#
# * represent types (it is a <i>type system</i>). These representations will be
#   referred to as _types_ in the documentation.
# * manipulate in-memory values represented by these types. These are
#   referred to as _values_ in the documentation.
#
# As types may depend on each other (for instance, a structure depend on the
# types used to define its fields), Typelib maintains a consistent set of types
# in a so-called registry. Types in a registry can only refer to other types in
# the same registry.
#
# On the Ruby side, a _type_ is represented as a subclass of one of the
# specialized subclasses of Typelib::Type (depending of what kind of type it
# is). I.e.  a _type_ itself is a class, and the methods that are available on
# Type objects are the singleton methods of the Type class (or its specialized
# subclasses).  Then, a value is simply an instance of that same class.
#
# Typelib specializes for the following kinds of types:
#
# * structures and unions (Typelib::CompoundType)
# * static length arrays (Typelib::ArrayType)
# * dynamic containers (Typelib::ContainerType)
# * mappings from strings to numerical values (Typelib::EnumType)
#
# In other words:
#
#   registry = <load the registry>
#   type  = registry.get 'A' # Get the Type subclass that represents the A
#                            # structure
#   value = type.new         # Create an uninitialized value of type A
#
#   value.class == type # => true
#   type.ancestors # => [type, Typelib::CompoundType, Typelib::Type]
#
# Each class representing a type can be further specialized using
# Typelib.specialize_model and Typelib.specialize
# 
module Typelib
    class << self
        # A type name to module mapping of the specializations defined by
        # Typelib.specialize
        attr_reader :value_specializations
        # A type name to module mapping of the specializations defined by
        # Typelib.specialize_model
        attr_reader :type_specializations
        # A [ruby class, type name] to [options, block] mapping of the custom convertions
        # defined by Typelib.convert_from_ruby
        attr_reader :convertions_from_ruby
        # A type name to [options, ruby_class, block] mapping of the custom convertions
        # defined by Typelib.convert_to_ruby. The ruby class might be nil if
        # it has not been specified
        attr_reader :convertions_to_ruby
	# If true (the default), typelib will load its type plugins. Otherwise,
        # it will not
        attr_predicate :load_type_plugins, true
    end

    @load_type_plugins = true
    @value_specializations = Hash.new
    @type_specializations = Hash.new
    @convertions_from_ruby = Hash.new { |h, k| h[k] = Array.new }
    @convertions_to_ruby = Hash.new { |h, k| h[k] = Array.new }

    # Adds methods to the type objects.
    #
    # The objects returned by registry.get(type_name) are themselves classes.
    # This method allows to define singleton methods, i.e. methods that will be
    # available on the type objects returned by Registry#get
    #
    # See Typelib.specialize to add instance methods to the values of a given
    # Typelib type
    def self.specialize_model(name, options = Hash.new, &block)
        options = Kernel.validate_options options, :if => lambda { |t| true }
        type_specializations[name] ||= Array.new
        type_specializations[name] << [options, Module.new(&block)]
    end

    class << self
        # Controls globally if Typelib.convert_to_ruby should use DynamicWrapper
        # on the return values or not
        #
        # See Typelib.convert_to_ruby for more information
        attr_accessor :use_dynamic_wrappers
    end
    @use_dynamic_wrappers = false


    # Internal proxy class that is used to offer a nice way to get hold on types
    # in #specialize blocks.
    #
    # The reason of the existence of that class is that, in principle,
    # specialize block should not rely on the global availability of a type,
    # i.e. they should not rely on the fact that the underlying type registry
    # has been exported in a particular namespace.
    #
    # However, it also means that in every method in specialize blocks, one
    # would have to do type tricks like
    #
    #   self['field_name'].deference.deference
    #
    # which is not nice.
    #
    # This class is used so that the following scheme is possible instead:
    #
    #   Typelib.specialize type_name do
    #      Subtype = self['field_name']
    #
    #      def my_method
    #        Subtype.new
    #      end
    #   end
    #
    class TypeSpecializationModule < Module # :nodoc:
        def included(obj)
            @base_type = obj
        end

        class TypeDefinitionAccessor # :nodoc:
            def initialize(specialization_module, ops)
                @specialization_module = specialization_module
                @ops = ops
            end

            def deference
                TypeDefinitionAccessor.new(@specialization_module, @ops + [[:deference]])
            end

            def [](name)
                TypeDefinitionAccessor.new(@specialization_module, @ops + [[:[], name]])
            end

            def method_missing(*mcall, &block)
                if !@type
                    base_type = @specialization_module.instance_variable_get(:@base_type)
                    if base_type
                        @type = @ops.inject(base_type) do |type, m|
                            type.send(*m)
                        end
                    end
                end

                if !@type
                    super
                else
                    @type.send(*mcall, &block)
                end
            end
        end

        def deference
            TypeDefinitionAccessor.new(self, [[:deference]])
        end

        def [](name)
            TypeDefinitionAccessor.new(self, [[:[], name]])
        end
    end

    # Extends instances of a given Typelib type
    #
    # This method allows to add methods that are then available on Typelib
    # values.
    #
    # For instance, if we assume that a Vector3 type is defined by
    #
    #   struct Vector3
    #   {
    #     double data[3];
    #   };
    #
    # Then
    #
    #   Typelib.specialize '/Vector3' do
    #     def +(other_v)
    #       result = new
    #       3.times do |i|
    #         result.data[i] = data[i] + other_v.data[i]
    #       end
    #     end
    #   end
    #
    # will make it possible to add two values of the Vector3 type in Ruby
    def self.specialize(name, options = Hash.new, &block)
        options = Kernel.validate_options options, :if => lambda { |t| true }
        value_specializations[name] ||= Array.new
        value_specializations[name] << [options, TypeSpecializationModule.new(&block)]
    end

    # Declares how to convert values of the given type to an equivalent Ruby
    # object
    #
    # For instance, given a hypothetical timeval type that would be defined (in C) by
    #
    #   struct timeval
    #   {
    #       int32_t seconds;
    #       uint32_t microseconds;
    #   };
    #
    # one could make sure that timeval values get automatically converted to
    # Ruby's Time with
    #
    #   Typelib.convert_to_ruby '/timeval' do |value|
    #     Time.at(value.seconds, value.microseconds)
    #   end
    #
    #
    # Optionally, for documentation purposes, it is possible to specify in what
    # type will the Typelib be converted:
    #
    #   Typelib.convert_to_ruby '/timeval', Time do |value|
    #     Time.at(value.seconds, value.microseconds)
    #   end
    #
    # The returned value might be wrapped using Typelib::DynamicWrapper. What it
    # means is that the returned value will behave as both the Typelib *and*
    # the converted value. This is turned off by default. It can be turned
    # globally on by setting Typelib.use_dynamic_wrappers to true, or on a
    # per-type basis by giving the :dynamic_wrapper => true option
    #
    # For instance, given
    #
    #   timeval_t = registry.get 'timeval' # get the timeval type
    #   timeval = timeval_t.new # an object of type timeval_t
    #   time = Typelib.to_ruby(timeval)
    #
    # Then both the following expressions are valid
    #
    #   time.tv_sec # defined as a Time method
    #
    # and
    #
    #   time.seconds # defined as field of the timeval structure
    def self.convert_to_ruby(typename, to = nil, options = Hash.new, &block)
        if to.kind_of?(Hash)
            to, options = nil, options
        end

        options = Kernel.validate_options options,
            :dynamic_wrappers => Typelib.use_dynamic_wrappers, :if => lambda { |t| true }
        if to && !to.kind_of?(Class)
            raise ArgumentError, "expected a class as second argument, got #{to}"
        elsif !typename.respond_to?(:to_str)
            raise ArgumentError, "expected a Typelib typename as first argument, got #{typename}"
        end

        convertions_to_ruby[typename] << [options, to, block]
    end

    # Define specialized convertions from Ruby objects to Typelib-managed
    # values.
    #
    # For instance, to allow the usage of Time instances to initialize structure
    # fields of the timeval type presented in Typelib.specialize_model, one
    # would do
    #
    #   Typelib.convert_from_ruby Time, '/timeval' do |value, typelib_type|
    #     v = typelib_type.new
    #     v.seconds      = value.tv_sec
    #     v.microseconds = value.tv_usec
    #   end
    #
    # It will then be possible to do
    #
    #   a.time = Time.now
    #
    # where 'a' is a value of a structure that has a 'time' field of the timeval
    # type, as for instance
    #
    #   struct A
    #   {
    #     timeval time;
    #   };
    #
    def self.convert_from_ruby(ruby_class, typename, options = Hash.new, &block)
        options = Kernel.validate_options options, :if => lambda { |t| true }
        if !ruby_class.kind_of?(Class)
            raise ArgumentError, "expected a class as first argument, got #{ruby_class}"
        elsif !typename.respond_to?(:to_str)
            raise ArgumentError, "expected a Typelib typename as second argument, got #{typename}"
        end
        convertions_from_ruby[typename] << [options, ruby_class, lambda(&block)]
    end 

    # The namespace separator character used by Typelib
    NAMESPACE_SEPARATOR = '/'

    # Returns the basename part of +name+, i.e. the type name
    # without the namespace part.
    #
    # See also Type.basename
    def self.basename(name, separator = Typelib::NAMESPACE_SEPARATOR)
        name = do_basename(name)
        if separator && separator != Typelib::NAMESPACE_SEPARATOR
            name.gsub!(Typelib::NAMESPACE_SEPARATOR, separator)
        end
        name
    end

    # Returns the namespace part of +name+.  If +separator+ is
    # given, the namespace components are separated by it, otherwise,
    # the default of Typelib::NAMESPACE_SEPARATOR is used. If nil is
    # used as new separator, no change is made either.
    def self.namespace(name, separator = Typelib::NAMESPACE_SEPARATOR, remove_leading = false)
        ns = do_namespace(name)
        if remove_leading
            ns = ns[1..-1]
        end
        if separator && separator != Typelib::NAMESPACE_SEPARATOR
            ns.gsub!(Typelib::NAMESPACE_SEPARATOR, separator)
        end
        ns
    end

    def self.define_method_if_possible(on, reference_class, name, allowed_overloadings = [], msg_name = nil, &block)
        if !reference_class.method_defined?(name) || allowed_overloadings.include?(name)
            on.send(:define_method, name, &block)
            true
        else
            msg_name ||= "instances of #{reference_class.name}"
            STDERR.puts "WARN: NOT defining #{name} on #{msg_name} as it would overload a necessary method"
            false
        end
    end

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

            def define_method_if_possible(name, &block)
                Typelib.define_method_if_possible(self, self, name, Type::ALLOWED_OVERLOADINGS, &block)
            end
        end
        @convertions_from_ruby = Hash.new

        # True if this type refers to subtype of the given type, or if it a
        # subtype of +type+ itself
        def self.contains?(type)
            self <= type ||
                dependencies.any? { |t| t.contains?(type) }
        end

        # Extends this type class to have values of this type converted by the
        # given block on C++/Ruby bondary
        def self.convert_to_ruby(to = nil, options = Hash.new, &block)
            options = Kernel.validate_options options,
                :use_dynamic_wrappers => Typelib.use_dynamic_wrappers,
                :recursive => true

            block = lambda(&block)
            m = Module.new do
                if options[:use_dynamic_wrappers]
                    define_method(:to_ruby) do |value|
                        Typelib::DynamicWrapper(block.call(value), value)
                    end
                else
                    define_method(:to_ruby, &block)
                end
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

        def self.find_custom_convertions(convertion_set, name = self.name)
            if convertion_set.has_key?(name)
                convertion_set[name].find_all do |options, _|
                    options[:if].call(self)
                end
            else
                []
            end
        end

        # Called by Typelib when a subclass is created.
        def self.subclass_initialize
            mods = find_custom_convertions(Typelib.type_specializations)
            mods.each do |opts, m|
                extend m
            end

            mods = find_custom_convertions(Typelib.value_specializations)
            mods.each do |opts, m|
                include m
            end

            @convertions_from_ruby = Hash.new
            convertions = find_custom_convertions(Typelib.convertions_from_ruby)
            convertions.each do |options, ruby_class, block|
                convert_from_ruby(ruby_class, &block)
            end

            convertions = find_custom_convertions(Typelib.convertions_to_ruby)
            convertions.each do |options, ruby_class, block|
                convert_to_ruby(ruby_class, &block)
                break
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

            def pretty_print(pp) # :nodoc:
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
	       	__real_new__(ptr) 
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
		other == self.to_ruby
	    end
        end

	# Returns a PointerType object which points to +self+. Note that
	# to_ptr.deference == self
        def to_ptr
            pointer = self.class.to_ptr.wrap(@ptr.to_ptr)
	    pointer.instance_variable_set(:@points_to, self)
	    pointer
        end
	
	alias :__to_s__ :to_s
	def to_s # :nodoc:
	    if respond_to?(:to_str)
		to_str
	    elsif ! (ruby_value = to_ruby).eql?(self)
		ruby_value.to_s
	    else
		__to_s__
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

    # Base class for numeric types
    class NumericType < Type
        def self.subclass_initialize
            super if defined? super

            if integer?
                # This is only a hint for the rest of Typelib. The actual
                # convertion is done internally by Typelib
                self.convertion_to_ruby = [Numeric, { :recursive => false }]
            else
                self.convertion_to_ruby = [Float, { :recursive => false }]
            end
        end

        def self.from_ruby(value)
            v = self.new
            v.typelib_from_ruby(value)
            v
        end
    end

    # Base class for opaque types
    class OpaqueType < Type
    end

    # Set of classes that have a #dup method but on which dup is forbidden
    DUP_FORBIDDEN = [TrueClass, FalseClass, Fixnum, Float, Symbol]

    # Base class for compound types (structs, unions)
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class CompoundType < Type
        # Creates a module that can be used to extend a certain Type class to
        # take into account the convertions.
        #
        # I.e. if a convertion is declared as
        #
        #   convert_to_ruby '/base/Time', :to => Time
        # 
        # and the type T is a structure with a field of type /base/Time, then
        # if
        #
        #   type = registry.get('T')
        #   type.extend_for_custom_convertions
        #
        # then
        #   t = type.new
        #   t.time => Time instance
        #   t.time => the same Time instance
        def self.extend_for_custom_convertions
            super if defined? super

            if !converted_fields.empty?
                self.contains_converted_types = true
                # Make it local so that it can be accessed in the module we define below
                converted_fields = self.converted_fields
                type_klass = self

                m = Module.new do
                    converted_fields.each do |field_name|
                        attr_name = "@#{field_name}"
                        Typelib.define_method_if_possible(self, type_klass, "#{field_name}=", Type::ALLOWED_OVERLOADINGS) do |value|
                            instance_variable_set(attr_name, value)
                        end
                        Typelib.define_method_if_possible(self, type_klass, field_name, Type::ALLOWED_OVERLOADINGS) do
                            v = instance_variable_get(attr_name)
                            if !v.nil?
                                v
                            else
                                v = get_field(field_name)
                                instance_variable_set(attr_name, v)
                            end
                        end
                    end

                    define_method(:invalidate_changes_from_converted_types) do
                        super()
                        converted_fields.each do |field_name|
                            instance_variable_set("@#{field_name}", nil)
                            if @fields[field_name]
                                @fields[field_name].invalidate_changes_from_converted_types
                            end
                        end
                    end

                    define_method(:apply_changes_from_converted_types) do
                        super()
                        converted_fields.each do |field_name|
                            value = instance_variable_get("@#{field_name}")
                            if !value.nil?
                                if @fields[field_name]
                                    @fields[field_name].apply_changes_from_converted_types
                                end
                                set_field(field_name, value)
                            end
                        end
                    end

                    define_method(:dup) do
                        new_value = super()
                        for field_name in converted_fields
                            converted_value = instance_variable_get("@#{field_name}")
                            if !converted_value.nil?
                                # false, nil,  numbers can't be dup'ed
                                if !DUP_FORBIDDEN.include?(converted_value.class)
                                    converted_value = converted_value.dup
                                end
                                instance_variable_set("@#{field_name}", converted_value)
                            end
                        end
                        new_value
                    end
                end
                include(m)
            end
        end

        # Internal method used to initialize a compound from a hash
        def set_hash(hash) # :nodoc:
            hash.each do |field_name, field_value|
                set_field(field_name, field_value)
            end
        end

        # Internal method used to initialize a compound from an array. The array
        # elements are supposed to be given in the field order
        def set_array(array) # :nodoc:
            fields = self.class.fields
            array.each_with_index do |value, i|
                set_field(fields[i][0], value)
            end
        end

	# Initializes this object to the pointer +ptr+, and initializes it
	# to +init+. Valid values for +init+ are:
	# * a hash, in which case it is a { field_name => field_value } hash
	# * an array, in which case the fields are initialized in order
	# Note that a compound should be either fully initialized or not initialized
        def initialize(ptr)
	    # A hash in which we cache Type objects for each of the structure fields
	    @fields = Hash.new
            @field_types = self.class.field_types

            super(ptr)
        end

        def raw_each_field
            self.class.each_field do |field_name, _|
                yield(field_name, raw_get_field(field_name))
            end
        end

        def each_field
            self.class.each_field do |field_name, _|
                yield(field_name, get_field(field_name))
            end
        end

        class << self
	    # Check if this type can be used in place of +typename+
	    # In case of compound types, we check that either self, or
	    # the first element field is +typename+
	    def is_a?(typename)
		super || self.fields[0].last.is_a?(typename)
	    end

            # The set of fields that are converted to a different type when
            # accessed from Ruby, as a set of names
            attr_reader :converted_fields

	    # Called by the extension to initialize the subclass
	    # For each field, it creates getters and setters on 
	    # the object, and a getter in the singleton class 
	    # which returns the field type
            def subclass_initialize
                @field_types = Hash.new
                @fields = get_fields.map! do |name, offset, type|
                    field_types[name] = type
                    field_types[name.to_sym] = type
                    [name, type]
                end

                @converted_fields = []
                each_field do |name, type|
                    if type.contains_converted_types?
                        converted_fields << name
                    end
                end

                fields = @fields
                converted_fields = @converted_fields
                fields.each do |name, type|
                    if !converted_fields.include?(name)
                        define_method_if_possible(name) { get_field(name) }
                        define_method_if_possible("#{name}=") { |value| set_field(name, value) }
                    end
                    define_method_if_possible("raw_#{name}") { raw_get_field(name) }
                    define_method_if_possible("raw_#{name}=") { |value| raw_set_field(name, value) }
                end

                super if defined? super

                convert_from_ruby Hash do |value, expected_type|
                    result = expected_type.new
                    result.set_hash(value)
                    result
                end

                convert_from_ruby Array do |value, expected_type|
                    result = expected_type.new
                    result.set_array(value)
                    result
                end
            end

            # Returns the offset, in bytes, of the given field
            def offset_of(fieldname)
                fieldname = fieldname.to_str
                get_fields.each do |name, offset, _|
                    return offset if name == fieldname
                end
                raise "no such field #{fieldname} in #{self}"
            end

	    # The list of fields
            attr_reader :fields
            # A name => type map of the types of each fiel
            attr_reader :field_types
	    # Returns the type of +name+
            def [](name)
                if result = @field_types[name]
                    result
                else
                    raise ArgumentError, "#{name} is not a field of #{self.name}"
                end
            end
            # True if the given field is defined
            def has_field?(name)
                @field_types.has_key?(name)
            end
	    # Iterates on all fields
            def each_field # :yield:name, type
		@fields.each { |field| yield(*field) } 
	    end

	    def pretty_print_common(pp) # :nodoc:
                pp.group(2, '{', '}') do
		    pp.breakable
                    all_fields = get_fields.to_a
                    
                    pp.seplist(all_fields) do |field|
			yield(*field)
                    end
                end
	    end

            def pretty_print(pp, verbose = false) # :nodoc:
		super(pp)
		pp.text ' '
		pretty_print_common(pp) do |name, offset, type|
		    pp.text name
                    if verbose
                        pp.text "[#{offset}] <"
                    else
                        pp.text " <"
                    end
		    pp.nest(2) do
                        type.pretty_print(pp)
		    end
		    pp.text '>'
		end
            end
        end

	def pretty_print(pp) # :nodoc:
	    self.class.pretty_print_common(pp) do |name, offset, type|
		pp.text name
		pp.text "="
		get_field(name).pretty_print(pp)
	    end
	end

        # Returns true if +name+ is a valid field name. It can either be given
        # as a string or a symbol
        def has_field?(name)
            @field_types.has_key?(name)
        end

        def [](name)
            get_field(name)
        end

	# Returns the value of the field +name+
        def get_field(name)
            if !has_field?(name)
                raise ArgumentError, "#{self.class.name} has no field called #{name}"
            end

            value = raw_get_field(name.to_s)
            Typelib.to_ruby(value, @field_types[name])
	end

        def raw_get_field(name)
            raw_get(name)
        end

        def raw_get(name)
	    if !(value = @fields[name])
		value = typelib_get_field(name)
		if value.kind_of?(Type)
		    @fields[name] = value
		end
	    end
            value
        end

        def raw_set_field(name, value)
            attribute = @fields[name]
            # If +value+ is already a typelib value, just do a plain copy
            if attribute && attribute.kind_of?(Typelib::Type) && value.kind_of?(Typelib::Type)
                return Typelib.copy(attribute, value)
            end
            typelib_set_field(name, value)

	rescue ArgumentError => e
	    if e.message =~ /^no field \w+ in /
		raise e, (e.message + " in #{name}(#{self.class.name})"), e.backtrace
	    else
		raise e, (e.message + " while setting #{name} in #{self.class.name}"), e.backtrace
	    end
        end

        def []=(name, value)
            raw_set_field(name, value)
        end

        # Sets the value of the field +name+. If +value+ is a hash, we expect
        # that the field is a compound type and initialize it using the keys of
        # +value+ as field names
        def set_field(name, value)
            if !has_field?(name)
                raise ArgumentError, "#{self.class.name} has no field called #{name}"
            end

            value = Typelib.from_ruby(value, @field_types[name])
            raw_set_field(name.to_s, value)

	rescue TypeError => e
	    raise e, "#{e.message} for #{self.class.name}.#{name}", e.backtrace
	end
    end

    class IndirectType < Type
    end

    # Base class for static-length arrays
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class ArrayType < IndirectType
        attr_reader :element_t

        def initialize(*args)
            super
            @element_t = self.class.deference
        end

        def self.find_custom_convertions(conversion_set)
            generic_array_id = deference.name + '[]'
            super(conversion_set) +
                super(conversion_set, generic_array_id)
        end

	def pretty_print(pp) # :nodoc:
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

            do_each do |el|
                yield(el)
            end
        end

        def each
            if !block_given?
                return enum_for(:each)
            end

            do_each do |el|
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

        def raw_get(index)
            do_get(index)
        end

        def [](index, range = nil)
            if range
                result = []
                range.times do |i|
                    result << Typelib.to_ruby(do_get(i + index), element_t)
                end
                result
            else
                Typelib.to_ruby(do_get(index), element_t)
            end
        end

        def []=(index, value)
            do_set(index, Typelib.from_ruby(value, element_t))
        end

	# Returns the pointed-to type (defined for consistency reasons)
	def self.[](index); deference end

        include Enumerable
    end

    # Base class for pointer types
    #
    # When returned as fields of a structure, or as return values from a
    # function, pointers might be converted in the following cases:
    # * nil if it is NULL
    # * a String object if it is a pointer to char
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class PointerType < IndirectType
        # Creates and initializes to zero a value of this pointer type
        def self.create_null
            result = new
            result.zero!
            result
        end

        def typelib_to_ruby
            if null? then nil
            else self
            end
        end
    end

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
        end

        # Module included in container types that offer random access
        # functionality
        module RandomAccessContainer
            def raw_get(index)
                if index < 0 || index >= size
                    raise ArgumentError, "index out of bounds"
                end

                do_get(index)
            end

            # Returns the value at the given index
            def [](index, chunk_size = nil)
                if chunk_size
                    if index < 0 || (index + chunk_size) > size
                        raise ArgumentError, "index out of bounds"
                    end
                    result = self.class.new
                    chunk_size.times do |i|
                        result.push(do_get(index + i))
                    end
                    result
                else
                    if index < 0 || index >= size
                        raise ArgumentError, "index out of bounds"
                    end

                    Typelib.to_ruby(do_get(index), element_t)
                end
            end

            def []=(index, value)
                if index < 0 || index >= size
                    raise ArgumentError, "index out of bounds"
                end

                do_set(index, Typelib.from_ruby(value, element_t))
            end
        end

        def self.subclass_initialize
            super if defined? super

            if random_access?
                include RandomAccessContainer
            end

            convert_from_ruby Array do |value, expected_type|
                t = expected_type.new
                value.each do |el|
                    t << el
                end
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
            STDERR.puts "WARN: Typelib::ContainerType#insert(value) is deprecated, use #push(value) instead"
            push(value)
        end

        # Adds a new value at the end of the sequence
        def push(*values)
            for v in values
                do_push(Typelib.from_ruby(v, element_t))
            end
        end

        def concat(array)
            push(*array)
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
        end

        # Deletes the elements
        def delete_if
            do_delete_if do |el|
                yield(Typelib.to_ruby(el, element_t))
            end
        end

        # True if this container is empty
        def empty?; length == 0 end

        # Appends a new element to this container
        def <<(value); push(value) end

        def pretty_print(pp)
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

    # Base class for all enumeration types. Enumerations
    # are mappings from strings to integers
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class EnumType < Type
        def self.from_ruby(value)
            v = new
            v.typelib_from_ruby(value)
            v
        end

        class << self
	    # A value => key hash for each enumeration values
            attr_reader :values

            def pretty_print(pp) # :nodoc:
                super
		pp.text '{'
                pp.nest(2) do
                    keys = self.keys.sort_by(&:last)
		    pp.breakable
                    pp.seplist(keys) do |keydef|
                        pp.text keydef.join(" = ")
                    end
                end
		pp.breakable
		pp.text '}'
            end
        end
    end

    def self.load_typelib_plugins
        if !ENV['TYPELIB_RUBY_PLUGIN_PATH'] || (@@typelib_plugin_path == ENV['TYPELIB_RUBY_PLUGIN_PATH'])
            return
        end

        ENV['TYPELIB_RUBY_PLUGIN_PATH'].split(':').each do |dir|
            specific_file = File.join(dir, "typelib_plugin.rb")
            if File.exists?(specific_file)
                require specific_file
            else
                Dir.glob(File.join(dir, '*.rb')) do |file|
                    require file
                end
            end
        end

        @@typelib_plugin_path = ENV['TYPELIB_RUBY_PLUGIN_PATH'].dup
    end
    @@typelib_plugin_path = nil
    

    # In Typelib, a registry contains a consistent set of types, i.e. the types
    # are that are related to each other.
    #
    # As mentionned in the Typelib module documentation, it is better to
    # manipulate value objects from types from the same registry. That is more
    # efficient, as it removes the need to compare the type definitions whenever
    # the values are manipulated together.
    #
    # I.e., it is better to use a global registry to represent all the types
    # used in your application. In case you need to load different registries,
    # that can be achieved by +merging+ them together (which will work only if
    # the type definitions match between the registries).
    class Registry
        TYPE_BY_EXT = {
            ".c" => "c",
            ".cc" => "c",
            ".cxx" => "c",
            ".cpp" => "c",
            ".h" => "c",
            ".hh" => "c",
            ".hxx" => "c",
            ".hpp" => "c",
            ".tlb" => "tlb"
        }

        TYPE_HANDLERS = Hash.new

        def dup
            copy = self.class.new
            copy.merge(self)
            copy
        end

	def minimal(type, with_aliases = true)
	    do_minimal(type, with_aliases)
	end

        # Creates a new registry by loading a typelib XML file
        #
        # See also Registry#merge_xml
        def self.from_xml(xml)
            reg = Typelib::Registry.new
            reg.merge_xml(xml)
            reg
        end

        def each(filter = nil, options = Hash.new, &block)
            if filter.kind_of?(Hash)
                filter, options = nil, filter
            end

            options = Kernel.validate_options options,
                :with_aliases => false

            if !block_given?
                enum_for(:each, filter, options)
            else
                each_type(filter, options[:with_aliases], &block)
            end
        end
        include Enumerable

        def include?(name)
            includes?(name)
        end

        attr_reader :exported_type_to_real_type
        attr_reader :real_type_to_exported_type

        def setup_type_export_module(mod)
            if !mod.respond_to?('find_exported_template')
                mod.extend(TypeExportNamespace)
                mod.registry = self
                mod.instance_variable_set :@exported_types, Hash.new
                mod.instance_variable_set :@exported_templates, Hash.new
            end
        end

        def export_solve_namespace(base_module, typename)
            namespace = Typelib.split_typename(typename)
            basename = namespace.pop

            setup_type_export_module(base_module)
            
            mod = namespace.inject(base_module) do |mod, ns|
                template_basename, template_args = GCCXMLLoader.parse_template(ns)
                ns = template_basename.gsub(/\s+/, '_').camelcase(:upper)

                if template_args.empty?
                    mod = 
                        if mod.const_defined_here?(ns)
                            mod.const_get(ns)
                        else
                            result = Module.new
                            mod.const_set(ns, result)
                            result
                        end

                    setup_type_export_module(mod)
                    mod
                else
                    # Must already be defined as it is an actual type object,
                    # not a namespace
                    template_args.map! do |s|
                        if s =~ /^\d+$/ then Integer(s)
                        else s
                        end
                    end
                    if !mod.respond_to?(:find_exported_template)
                        return
                    end
                    template = mod.find_exported_template(ns, [], template_args)
                    return if !template
                    template
                end
            end
            return [basename, mod]
        end

        attr_reader :export_typemap

        class InconsistentTypeExport < RuntimeError
            attr_reader :path
            attr_reader :existing_type
            attr_reader :new_type

            def initialize(path, existing_type, new_type, message = nil)
                super(message)
                @path = path
                @existing_type = existing_type
                @new_type = new_type
            end

            def exception(message = nil)
                if message.respond_to?(:to_str)
                    self.class.new(path, existing_type, new_type, message)
                else
                    self
                end
            end

            def pretty_print(pp)
                pp.text "there is a type registered at #{path} that differs from the one we are trying to register"
                pp.breakable
                pp.text "registered type is"
                pp.nest(2) do
                    pp.breakable
                    existing_type.pretty_print(pp)
                end
                pp.breakable
                pp.text "new type is"
                pp.nest(2) do
                    pp.breakable
                    new_type.pretty_print(pp)
                end
            end
        end

        # Export this registry in the Ruby namespace. The base namespace under
        # which it should be done is given in +base_module+
        def export_to_ruby(base_module = Kernel, options = Hash.new)
            options = Kernel.validate_options options,
                :override => false, :excludes => nil

            override   = options[:override]
            exclude_rx = /^$/
            if options[:excludes].respond_to?(:each)
                rx_elements = []
                options[:excludes].each do |s|
                    if s.respond_to?(:to_str)
                        rx_elements << "^#{Regexp.quote(s)}$"
                    elsif s.kind_of?(Regexp)
                        rx_elements << s.to_s
                    end
                end
                exclude_rx = Regexp.new(rx_elements.join("|"))
            elsif options[:excludes]
                exclude_rx = Regexp.new("^#{Regexp.quote(options[:excludes])}$")
            end

            new_export_typemap = Hash.new
            each(:with_aliases => true) do |name, type|
                next if name =~ exclude_rx

                basename, mod = export_solve_namespace(base_module, name)
                if !mod.respond_to?(:find_exported_template)
                    next
                end
                next if !mod

                exported_type =
                    if type.convertion_to_ruby
                        type.convertion_to_ruby[0] || type
                    else
                        type
                    end

                if block_given?
                    exported_type = yield(name, type, mod, basename, exported_type)
                    next if !exported_type
                    if !exported_type.kind_of?(Class)
                        raise ArgumentError, "the block given to export_to_ruby must return a Ruby class or nil (got #{exported_type})"
                    end

                    if exported_type.respond_to?(:convertion_to_ruby) && exported_type.convertion_to_ruby
                        exported_type = exported_type.convertion_to_ruby[0] || exported_type
                    end
                end

                new_export_typemap[exported_type] ||= Array.new
                new_export_typemap[exported_type] << [type, mod, basename]

                if (existing_exports = @export_typemap[exported_type]) && existing_exports.include?([type, mod, basename])
                    next
                end

                if type <= Typelib::PointerType || type <= Typelib::ArrayType
                    # We ignore arrays for now
                    # export_array_to_ruby(mod, $1, Integer($2), exported_type)
                    next
                end

                template_basename, template_args = GCCXMLLoader.parse_template(basename)
                if template_args.empty?
                    basename = basename.gsub(/\s+/, '_').camelcase(:upper)

                    if mod.const_defined_here?(basename)
                        existing_type = mod.const_get(basename)
                        if override
                            mod.const_set(basename, exported_type)
                        elsif !(existing_type <= exported_type)
                            raise InconsistentTypeExport.new("#{mod.name}::#{basename}", existing_type, exported_type), "there is a type registered at #{mod.name}::#{basename} which differs from the one in the registry, and override is false"
                        end
                    else
                        mod.exported_types[basename] = type
                        mod.const_set(basename, exported_type)
                    end
                else
                    export_template_to_ruby(mod, template_basename, template_args, type, exported_type)
                end
            end

            @export_typemap = new_export_typemap
        end

        # Remove all types exported by #export_to_ruby under +mod+, and
        # de-registers them from the export_typemap
        def clear_exports(export_target_mod)
            if !export_target_mod.respond_to?(:exported_types)
                return
            end

            found_type_exports = ValueSet.new
            
            export_target_mod.exported_types.each do |const_name, type|
                found_type_exports.insert(export_target_mod.const_get(const_name))
                export_target_mod.send(:remove_const, const_name)
            end
            export_target_mod.exported_types.clear
            export_target_mod.constants.each do |c|
                if c.respond_to?(:exported_types)
                    clear_exports(c)
                end
            end

            found_type_exports.each do |exported_type|
                set = export_typemap[exported_type]
                set.delete_if do |_, mod, _|
                    export_target_mod == mod
                end
                if set.empty?
                    export_typemap.delete(exported_type)
                    puts export_typemap.size
                end
            end
        end

        module TypeExportNamespace
            attr_accessor :registry
            attr_reader :exported_types
            attr_reader :exported_templates

            def find_exported_template(template_basename, args, remaining)
                if remaining.empty?
                    return @exported_templates[[template_basename, args]]
                end

                head, *tail = remaining

                # Accept Ruby types in place of the corresponding Typelib type,
                # and accept type objects in place of type names
                if head.kind_of?(Class)
                    if real_types = registry.export_typemap[head]
                        head = real_types.map do |t, _|
                            t.name
                        end
                    else
                        raise ArgumentError, "#{arg} is not a type exported from a Typelib registry"
                    end
                end

                if head.respond_to?(:each)
                    args << nil
                    for a in head
                        args[-1] = a
                        if result = find_exported_template(template_basename, args, tail)
                            return result
                        end
                    end
                    return nil
                else
                    args << head
                    return find_exported_template(template_basename, args, tail)
                end
            end
        end

        def export_template_to_ruby(mod, template_basename, template_args, actual_type, exported_type)
            template_basename = template_basename.gsub(/\s+/, '_').camelcase(:upper)

            if !mod.respond_to?(template_basename)
                mod.singleton_class.class_eval do
                    define_method(template_basename) do |*args|
                        if !(result = find_exported_template(template_basename, [], args))
                            raise ArgumentError, "there is no template instanciation #{template_basename}<#{args.join(",")}> available"
                        end
                        result
                    end
                end
            end

            template_args = template_args.map do |arg|
                if arg =~ /^\d+$/
                    Integer(arg)
                else
                    arg
                end
            end
            exports = mod.instance_variable_get :@exported_templates
            exports[[template_basename, template_args]] = exported_type
        end

	# Returns the file type as expected by Typelib from 
	# the extension of +file+ (see TYPE_BY_EXT)
	#
	# Raises RuntimeError if the file extension is unknown
        def self.guess_type(file)
	    ext = File.extname(file)
            if type = TYPE_BY_EXT[ext]
		type
            else
                raise "Cannot guess file type for #{file}: unknown extension '#{ext}'"
            end
        end

	# Format +option_hash+ to the form expected by do_import
	# (Yes, I'm lazy and don't want to handles hashes in C)
        def self.format_options(option_hash) # :nodoc:
            option_hash.to_a.collect do |opt|
                if opt[1].kind_of?(Array)
                    if opt[1].first.kind_of?(Hash)
                        [ opt[0].to_s, opt[1].map { |child| format_options(child) } ]
                    else
                        [ opt[0].to_s, opt[1].map { |child| child.to_s } ]
                    end
                elsif opt[1].kind_of?(Hash)
                    [ opt[0].to_s, format_options(opt[1]) ]
                else
                    [ opt[0].to_s, opt[1].to_s ]
                end
            end
        end

        # Shortcut for
        #   registry = Registry.new
        #   registry.import(args)
        #
        # See Registry#import
        def self.import(*args)
            registry = Registry.new
            registry.import(*args)
            registry
        end

        def initialize(load_plugins = nil)
	    if load_plugins || (load_plugins.nil? && Typelib.load_type_plugins?)
            	Typelib.load_typelib_plugins
            end
            @export_typemap = Hash.new
	    super()
        end

        # Returns the handler that will be used to import that file. It can
        # either be a string, in which case we use a Typelib internal importer,
        # or a Ruby object responding to 'call' in which case Registry#import
        # will use that object to do the importing.
        def self.handler_for(file, kind = 'auto')
	    file = File.expand_path(file)
            if !kind || kind == 'auto'
                kind    = Registry.guess_type(file)
            end
            if handler = TYPE_HANDLERS[kind]
                return handler
            end
            return kind
        end

        # Imports the +file+ into this registry. +kind+ is the file format or
        # nil, in which case the file format is guessed by extension (see
        # TYPE_BY_EXT)
	# 
        # +options+ is an option hash. The Ruby bindings define the following
        # specific options:
	# merge:: 
        #   merges +file+ into this repository. If this is false, an exception
        #   is raised if +file+ contains types already defined in +self+, even
        #   if the definitions are the same.
	#
	#     registry.import(my_path, 'auto', :merge => true)
	#
	# The Tlb importer has no options
	#
        # The C importer defines the following options: preprocessor:
        #
	# define:: 
        #   a list of VAR=VALUE or VAR options for cpp
	#     registry.import(my_path, :define => ['PATH=/usr', 'NDEBUG'])
	# include:: 
        #   a list of path to add to cpp's search path
	#     registry.import(my_path, :include => ['/usr', '/home/blabla/prefix/include'])
	# rawflags:: 
        #   flags to be passed as-is to cpp. For instance, the two previous
        #   examples can be written
	#
	#   registry.import(my_path, 'auto',
	#     :rawflags => ['-I/usr', '-I/home/blabla/prefix/include', 
	#                  -DPATH=/usr', -DNDEBUG])
	# debug::
        #   if true, debugging information is outputted on stdout, and the
        #   preprocessed output is kept.
        #
        # merge::
        #   load the file into its own registry, and merge the result back into
        #   this one. If it is not set, types defined in +file+ that are already
        #   defined in +self+ will generate an error, even if the two
        #   definitions are the same.
	#
        def import(file, kind = 'auto', options = {})
	    file = File.expand_path(file)

            handler = Registry.handler_for(file, kind)
            if handler.respond_to?(:call)
                return handler.call(self, file, kind, options)
            else
                kind = handler
            end

            do_merge = 
                if options.has_key?('merge') then options.delete('merge')
                elsif options.has_key?(:merge) then options.delete(:merge)
                else true
                end

            options = Registry.format_options(options)

            do_import(file, kind, do_merge, options)
        end

        # Resizes the given type to the given size, while updating the rest of
        # the registry to keep it consistent
        #
        # In practice, it means it modifies the compound field offsets and
        # sizes, and modifies the array sizes so that it matches the new sizes.
        #
        # +type+ must either be a type class or a type name, and to_size the new
        # size for it.
        #
        # See #resize to resize multiple types in one call.
        def resize_type(type, to_size)
            resize(type => to_size)
        end

        # Resize a set of types, while updating the rest of the registry to keep
        # it consistent
        #
        # In practice, it means it modifies the compound field offsets and
        # sizes, and modifies the array sizes so that it matches the new sizes.
        #
        # The given type map must be a mapping from a type name or type class to
        # the new size for that type.
        #
        # See #resize to resize multiple types in one call.
        def resize(typemap)
            new_sizes = typemap.map do |type, size|
                if type.respond_to?(:name)
                    [type.name, size]
                else
                    [type.to_str, size]
                end
            end

            do_resize(new_sizes)
            nil
        end

	# Exports the registry in the provided format, into a Ruby string. The
	# following formats are allowed as +format+:
	# 
	# +tlb+:: Typelib's own XML format
	# +idl+:: CORBA IDL
	# 
	# +options+ is an option hash, which is export-format specific. See the C++
	# documentation of each exporter for more information.
	def export(kind, options = {})
            options = Registry.format_options(options)
            do_export(kind, options)
	end

        # Export the registry into Typelib's own XML format
        def to_xml
            export('tlb')
        end
    end

    # Convenience registry class that adds the builtin C++ types at construction
    # time
    class CXXRegistry < Registry
        def initialize
            super
            Registry.add_standard_cxx_types(self)
        end
    end

    # This module is used to extend base Ruby types so that they behave like
    # their corresponding Typelib types 
    #
    # I.e. it must be possible to do
    #
    #   structure.time.seconds = 10
    #
    # while, still, having the option of doing
    #
    #   structure.time => Time instance
    module DynamicWrapperBase
        def to_ruby
            @ruby_object
        end
        def to_typelib
            @typelib_object
        end

        def pretty_print(pp)
            pp @typelib_object
        end

        def method_missing(m, *args, &block)
            if @typelib_object.respond_to?(m)
                return @typelib_object.send(m, *args, &block)
            end
            super
        end
    end

    def self.DynamicWrapper(ruby_object, typelib_object)
        ruby_object.extend DynamicWrapperBase
        ruby_object.instance_variable_set(:@ruby_object,    ruby_object)
        ruby_object.instance_variable_set(:@typelib_object, typelib_object)
        ruby_object
    end
end

require 'typelib_ruby'

module Typelib
    # Get the name for 'char'
    reg = Registry.new(false)
    Registry.add_standard_cxx_types(reg)
    CHAR_T = reg.get('/char')

    convert_from_ruby TrueClass, '/bool' do |value, typelib_type|
        Typelib.from_ruby(1, typelib_type)
    end
    convert_from_ruby FalseClass, '/bool' do |value, typelib_type|
        Typelib.from_ruby(0, typelib_type)
    end
    convert_to_ruby '/bool' do |value|
        value != 0
    end

    convert_from_ruby String, '/std/string' do |value, typelib_type|
        typelib_type.wrap([value.length, value].pack("QA#{value.length}"))
    end
    convert_to_ruby '/std/string', String do |value|
        value.to_byte_array[8..-1]
    end
    specialize '/std/string' do
        def to_str
            Typelib.to_ruby(self)
        end

        def concat(other_string)
            super(Typelib.from_ruby(other_string, self.class))
        end
    end

    ####
    # C string handling
    convert_from_ruby String, CHAR_T.name do |value, typelib_type|
        if value.size != 1
            raise ArgumentError, "trying to convert a string of length different than one to a character"
        end
        Typelib.from_ruby(value[0], typelib_type)
    end
    convert_from_ruby String, "#{CHAR_T.name}[]" do |value, typelib_type|
        result = typelib_type.new
        Type::from_string(result, value)
        result
    end
    convert_to_ruby "#{CHAR_T.name}[]", String do |value|
        Type::to_string(value)
    end
    specialize "#{CHAR_T.name}[]" do
        def to_str
            Type::to_string(self)
        end
    end
    convert_from_ruby String, "#{CHAR_T.name}*" do |value, typelib_type|
        result = typelib_type.new
        Type::from_string(result, value)
        result
    end
    convert_to_ruby "#{CHAR_T.name}*", String do |value|
        Type::to_string(value)
    end
    specialize "#{CHAR_T.name}*" do
        def to_str
            Type::to_string(self)
        end
    end
end

module Typelib
    # Generic method that converts a Typelib value into the corresponding Ruby
    # value.
    def self.to_ruby(value, original_type = nil)
        if value.respond_to?(:typelib_to_ruby)
            value = value.typelib_to_ruby
        end

        if (t = (original_type || value.class)).respond_to?(:to_ruby)
            # This allows to override Typelib's internal type convertion (mainly
            # for numerical types).
            t.to_ruby(value)
        else
            value
        end
    end

    # call-seq:
    #  Typelib.copy(to, from) => to
    # 
    # Proper copy of a value to another. +to+ and +from+ do not have to be from the
    # same registry, as long as the types can be casted into each other
    def self.copy(to, from)
        if to.respond_to?(:invalidate_changes_from_converted_types)
            to.invalidate_changes_from_converted_types
        end
        if from.respond_to?(:apply_changes_from_converted_types)
            from.apply_changes_from_converted_types
        end
        do_copy(to, from)
        to
    end

    # Initializes +expected_type+ from +arg+, where +arg+ can either be a value
    # of expected_type, a value that can be casted into a value of
    # expected_type, or a Ruby value that can be converted into a value of
    # +expected_type+.
    def self.from_ruby(arg, expected_type)      
        if arg.respond_to?(:apply_changes_from_converted_types)
            arg.apply_changes_from_converted_types
        end

        if arg.kind_of?(expected_type)
            return arg
        elsif arg.class < Type && arg.class.casts_to?(expected_type)
            return arg.cast(expected_type)
        elsif convertion = expected_type.convertions_from_ruby[arg.class]
            converted = convertion.call(arg, expected_type)
        elsif expected_type.respond_to?(:from_ruby)
            converted = expected_type.from_ruby(arg)
        else
            if !(expected_type < NumericType) && !arg.kind_of?(expected_type)
                raise ArgumentError, "cannot convert #{arg} to #{expected_type.name}"
            end
            converted = arg
        end
        if !(expected_type < NumericType) && !converted.kind_of?(expected_type)
            raise InternalError, "invalid conversion of #{arg} to #{expected_type.name}"
        end

        converted
    end

    # A raw, untyped, memory zone
    class MemoryZone
	def to_s
	    "#<MemoryZone:#{object_id} ptr=0x#{address.to_s(16)}>"
	end
    end
end

if Typelib.with_dyncall?
module Typelib
    # An opened C library
    class Library
	# The file name of this library
	attr_reader :name
	# The type registry for this library
        attr_reader :registry

	class << self
	    private :new
	end

	# Open the library at +libname+, associated with the type registry
	# +registry+. This returns a Library object.
        #
        # If no registry a given, an empty one is created.
        #
        # If auto_unload is true (the default), the shared object gets unloaded
        # when the Library object is deleted. Otherwise, it will never be
        # unloaded.
	def self.open(libname, registry = nil, auto_unload = true)
	    libname = File.expand_path(libname)
	    lib = wrap(libname, auto_unload)
            registry ||= Registry.new
	    lib.instance_variable_set("@name", libname)
	    lib.instance_variable_set("@registry", registry)
	    lib
	end
    end

    # A function found in a Typelib::Library. See Library#find
    class Function
	# The library in which this function is defined
	attr_reader :library
	# The function name
	attr_reader :name

	# The return type of this function, or nil if there is no return value
	attr_reader :return_type
	# The set of argument types.
	attr_reader :arguments
	# The set of argument indexes which contain returned data. See #modifies and #returns.
	attr_reader :returned_arguments

	def initialize(library, name)
	    @library = library
	    @name = name
	    @arguments = []
	    @returned_arguments = []
	    @arity = 0
	end

	# Specifies that this function returns a value of type +typename+. The
	# first call to +returns+ specifies the return value of the function
	# (in the C sense).  Subsequent calls specify that the following
	# argument is of type +typename+, and that the argument is a buffer
	# whose sole purpose is to be filled by the function call. In this case,
	# the buffer does not have to be given to #call: the Function object will create
	# one on the fly and #call will return it.
	#
	# For instance, the following standard C function:
	#
	#   int gettimeofday(struct timeval* tv, struct timezone* tz);
	#
	# Is wrapped using
	#
	#   gettimeofday = libc.find('gettimeofday').
	#	returns('int').
	#	returns('struct timeval*').
	#	with_arguments('struct timezone*')
	#
	# And called using
	#
	#   status, time = gettimeofday.call(nil)
	#
	# If the function does not have a return type but *has* a returned argument,
	# the return type must be explicitely specified as 'nil':
	#
	#   my_function.returns(nil).returns('int*')
	#
	def returns(typename)
	    if typename
		type = library.registry.build(typename)
		if type.null?
		    type = nil
		end
	    end

	    if defined? @return_type
		if !type
		    raise ArgumentError, "null type specified as argument"
		end

		@arguments << type
		@returned_arguments << @arguments.size
	    elsif typename
		@return_type = type
	    else
		@return_type = nil
	    end
	    self
	end

	# Specify the type of the following arguments
	def with_arguments(*typenames)
	    types = typenames.map { |n| library.registry.build(n) }
	    @arguments.concat(types)
	    @arity += typenames.size
	    self
	end

	# Specify that the following argument is a buffer which both provides
	# an argument to the function *and* will be modified by the function.
	# For instance, the select standard C function would be wrapped as
	# follows:
	#
	#   select = libc.find('select').
	#	returns('int').modifies('fd_set*').
	#	modifies('fd_set*').modifies('fd_set*').
	#	modifies('struct timeval*')
	#
	# and can then be used with:
	#
	#   status, read, write, except, timeout = 
	#	select.call(read, write, except, timeout)
	#
	# A feature of this library is the possibility to automatically build
	# memory buffers from Ruby values. So, for instance, the following C
	# function
	#
	#   void my_function(int* value);
	#
	# can be used as follows:
	#
	#   my_function = lib.wrap('my_function').modifies('int*')
	#
	#   new_value = my_function.call(5)
	#
	# The library will then automatically convert the Ruby integer '5' into
	# a C memory buffer of type 'int' and return the new Ruby value from
	# this buffer (new_value is a Ruby integer).
	def modifies(typename)
	    @arguments << library.registry.build(typename)
	    @returned_arguments << -@arguments.size
	    @arity += 1
	    self
	end

	# True if this function returns some value (either a return type or
	# some arguments which are also output values)
	def returns_something?; @return_type || !@returned_arguments.empty? end

	# How many arguments #call expects
	attr_reader :arity

	# Calls the function with +args+ for arguments
	def call(*args)
	    filtered_args, vm = prepare_call(args)
	    perform_call(filtered_args, vm)
	end

	alias :[] :call
	# Checks that +args+ is a valid set of arguments for this function
	def filter(*args); Typelib.filter_function_args(args, self) end
	alias :check :filter

	# Returns a CompiledCall object which binds this function to the
	# given argument. See CompiledCall#call
	def compile(*args)
	    filtered_args, vm = prepare_call(args)
	    CompiledCall.new(self, filtered_args, vm)
	end

	def prepare_call(args) # :nodoc:
	    filtered_args = Typelib.filter_function_args(args, self)
	    vm = prepare_vm(filtered_args)
	    return filtered_args, vm
	end

	def perform_call(filtered_args, vm) # :nodoc:
	    retval = vm.call(self)
	    return unless returns_something?

	    # Get the return array
	    ruby_returns = []
	    if return_type
		if MemoryZone === retval
		    ruby_returns << return_type.wrap(retval.to_ptr)
		else
		    ruby_returns << retval
		end
	    end

	    for index in returned_arguments
		ary_idx = index.abs - 1

		value = filtered_args[ary_idx]
		type  = arguments[ary_idx]

		if type < ArrayType
		    ruby_returns << type.wrap(value)
		else
		    ruby_returns << type.deference.wrap(value).to_ruby
		end
	    end

            if ruby_returns.size == 1
                return ruby_returns.first
            else
                return ruby_returns
            end
	end

    end

    # A CompiledCall object is a Function on which some arguments have already
    # been bound
    class CompiledCall
	def initialize(function, filtered_args, vm)
	    @function, @filtered_args, @vm = function, filtered_args, vm
	end

	def call
	    @function.perform_call(@filtered_args, @vm)
	end
    end

    # Implement Typelib's argument handling
    # This method filters a particular argument given the user-supplied value and 
    # the argument expected type. It raises TypeError if +arg+ is of the wrong type
    def self.filter_argument(arg, expected_type) # :nodoc:
	if !(expected_type < IndirectType) && !(Kernel.immediate?(arg) || Kernel.numeric?(arg))
	    raise TypeError, "#{arg.inspect} cannot be used for #{expected_type.name} arguments"
	end

	filtered =  if expected_type < IndirectType
			if MemoryZone === arg
			    arg
			elsif Type === arg
			    filter_value_arg(arg, expected_type)
                        elsif arg.nil?
                            expected_type.create_null
			elsif expected_type.deference.name == CHAR_T.name && arg.respond_to?(:to_str)
			    # Ruby strings ARE null-terminated
			    # The thing which is not checked here is that there is no NULL bytes
			    # inside the string.
			    arg.to_str.to_memory_ptr
			end
		    end

	if !filtered && (Kernel.immediate?(arg) || Kernel.numeric?(arg))
	    filtered = filter_numeric_arg(arg, expected_type)
	end

	if !filtered
          raise TypeError, "cannot use #{arg.inspect} for a #{expected_type.inspect} argument. Object ID for argument #{arg.class.object_id}, object ID for expected type #{expected_type.object_id}"
	end
	filtered
    end

    # Creates an array of objects that can safely be passed to function call mechanism
    def self.filter_function_args(args, function) # :nodoc:
        # Check we have the right count of arguments
        if args.size != function.arity
            raise ArgumentError, "#{function.arity} arguments expected, got #{args.size}"
        end
        
        # Create a Value object to collect the data we'll get
        # from output-only arguments
        function.returned_arguments.each do |index|
            next unless index > 0
            ary_idx = index - 1

	    type = function.arguments[ary_idx]
	    buffer = if type < ArrayType then type.new
		     elsif type < PointerType then type.deference.new
		     else
			 raise ArgumentError, "unexpected output type #{type.name}"
		     end

            args.insert(ary_idx, buffer) # works because return_spec is sorted
        end

	args.each_with_index do |arg, idx|
	    args[idx] = filter_argument(arg, function.arguments[idx])
	end
    end
end
end

if ENV['TYPELIB_USE_GCCXML'] != '0'
    require 'typelib-gccxml'
end

