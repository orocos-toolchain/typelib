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
    end

    # Class used to register type-to-object mappings.
    #
    # Customizations can eithe be registered as string-to-object or
    # regexp-to-object mappings. We have to go through all regexps in the second
    # case, so we split the two matching methods to speed up the common case
    # (strings)
    class RubyMappingCustomization
        # Mapping from type names to registered objects
        # @return [{String=>Object}]
        attr_reader :from_typename
        # Mapping from the element type name of an array
        # @return [{String=>Object}]
        attr_reader :from_array_basename
        # Mapping from the container type name
        # @return [{String=>Object}]
        attr_reader :from_container_basename
        # Mapping from regexps matching type names to registered objects
        # @return [{Regexp=>Object}]
        attr_reader :from_regexp
        # A stereotypical container that should be used as base object in
        # {from_regexp} and {from_type}. If nil, only one object can be
        # registered at the time
        attr_reader :container

        def initialize(container = nil)
            @from_typename = Hash.new
            @from_array_basename = Hash.new
            @from_container_basename = Hash.new
            @from_regexp = Hash.new
            @container = container
        end

        # Returns the right mapping object for that key
        #
        # @return [String,Hash] one of the from_* sets as well as the string
        #   that should be used as key
        def mapping_for_key(key)
            if key.respond_to?(:to_str)
                suffix = key[-2, 2]
                if suffix == "<>"
                    return key[0..-3], from_container_basename
                elsif suffix == "[]"
                    return key[0..-3], from_array_basename
                else
                    return key, from_typename
                end
            else return key, from_regexp
            end
        end

        # Sets the value for a given key
        #
        # @param [Regexp,String] the object that will be used to match the type
        #   name
        # @param [Object] value the value to be stored for that key
        def set(key, value, options = Hash.new)
            options = Kernel.validate_options options, :if => lambda { |obj| true }
            key, set = mapping_for_key(key)
            set = set[key] = (container || Array.new).dup
            set << [options, value]
        end

        # Add a value to a certain key. Note that this is only possible if a
        # container has been given at construction time
        #
        # @raise [ArgumentError] if this registration class does not support
        #   having more than one value per key
        #
        # @param [Regexp,String] the object that will be used to match the type
        #   name
        # @param [Object] value the value to be added to the set for that key
        def add(key, value, options = Hash.new)
            options = Kernel.validate_options options, :if => lambda { |obj| true }
            if !container
                raise ArgumentError, "#{self} does not support containers"
            end
            key, set = mapping_for_key(key)
            set = (set[key] ||= container.dup)
            set << [options, value]
        end

        # Returns the single object registered for the given type name
        #
        # @raise [ArgumentError] if more than one matching object is found
        def find(type_model, error_if_ambiguous = true)
            all = find_all(type_model)
            if all.size > 1 && error_if_ambiguous
                raise ArgumentError, "more than one entry matches #{name}"
            else all.first
            end
        end

        # Returns all objects matching the given type name
        #
        # @raise [Array<Object>]
        def find_all(type_model, name = type_model.name)
            # We first build a set of candidates, and then filter with the :if
            # block
            candidates = Array.new
            if type_model <= Typelib::ContainerType
                if set = from_container_basename[type_model.container_kind]
                    candidates.concat(set)
                end
            elsif type_model <= Typelib::ArrayType
                if set = from_array_basename[type_model.deference.name]
                    candidates.concat(set)
                end
            end
            if registered = from_typename[name]
                candidates.concat(registered)
            end
            from_regexp.each do |matcher, registered|
                if matcher === name
                    candidates.concat(registered)
                end
            end

            candidates.map do |options, obj|
                obj if options[:if].call(type_model)
            end.compact
        end
    end

    # Initialize the specialization-related attributes on the Typelib module
    @value_specializations = RubyMappingCustomization.new(Array.new)
    @type_specializations  = RubyMappingCustomization.new(Array.new)
    @convertions_from_ruby = RubyMappingCustomization.new(Array.new)
    @convertions_to_ruby   = RubyMappingCustomization.new(Array.new)

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
        type_specializations.add(name, Module.new(&block), options)
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
        value_specializations.add(name, TypeSpecializationModule.new(&block), options)
    end

    # Representation of a declared ruby-to-typelib or typelib-to-ruby convertion
    class Convertion
        # The type that we are converting from
        #
        # It is an object that can match type names
        #
        # @return [String,Regexp]
        attr_reader :typelib
        # The type that we are converting to, if known. It cannot be nil if this
        # object represents a ruby-to-typelib convertion
        #
        # @return [Class,nil]
        attr_reader :ruby
        # The convertion proc
        attr_reader :block

        def initialize(typelib, ruby, block)
            @typelib, @ruby, @block = typelib, ruby, block
        end
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
    def self.convert_to_ruby(typename, ruby_class = nil, options = Hash.new, &block)
        if ruby_class.kind_of?(Hash)
            ruby_class, options = nil, options
        end

        if ruby_class && !ruby_class.kind_of?(Class)
            raise ArgumentError, "expected a class as second argument, got #{to}"
        end
        convertions_to_ruby.add(typename, Convertion.new(typename, ruby_class, lambda(&block)), options)
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
        end
        convertions_from_ruby.add(typename, Convertion.new(typename, ruby_class, lambda(&block)), options)
    end
end

