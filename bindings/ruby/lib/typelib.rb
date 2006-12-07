require 'enumerator'
require 'utilrb/object/singleton_class'
require 'delegate'
require 'pp'
module DL
    class Handle
        def registry; @typelib_registry end
    end
end

module Typelib
    # Base class for all types
    # Registry types are wrapped into subclasses of Type
    # or other Type-derived classes (Array, Pointer, ...)
    #
    # Value objects are wrapped into instances of these classes
    class Type
        @writable = true

        class << self
	    # the type registry we belong to
            attr_reader :registry
	    def name
		if defined? @name
		    @name
		else
		    super
		end
	    end

	    # check if this is writable
            def writable?
                if !defined?(@writable)
                    superclass.writable?
                else
                    @writable
                end
            end
	    # are we a null type ?
	    def null?; @null end
	    # returns the pointer-to-self type
            def to_ptr; registry.build(name + "*") end

            def pretty_print(pp) # :nodoc:
		pp.text name 
	    end

            alias :__real_new__ :new
            def wrap(ptr); __real_new__(ptr) end
            def new; __real_new__(nil) end

	    # Check if this type is a +typename+. If +typename+
	    # is a string or a regexp, we match it against the type
	    # name. Otherwise we call Class::is_a?
	    def is_a?(typename)
		if typename.respond_to?(:to_str) || Regexp === typename
		    typename.to_str === self.name
		else
		    super
		end
	    end
        end

	# Module that gets mixed into types which can
	# be converted to strings (char* for instance)
	module StringHandler
	    def to_str
		Type::to_string(self)
	    end
	    def from_str(value)
		Type::from_string(self, value)
	    end
	end

	def initialize(*args)
	    __initialize__(*args)
	    extend StringHandler if string_handler?
	end

	# Check for value equality
        def ==(other)
	    # If other is also a type object, we first
	    # check basic constraints before trying conversion
	    # to Ruby objects
            if Type === other
		return false unless self.class.eql?(other.class)
		return memory_eql?(other)
	    else
		if (ruby_value = self.to_ruby).eql?(self)
		    return false
		end
		other == self.to_ruby
	    end
        end

	# returns a PointerType object which points to +self+. Note that
	# to_ptr.deference == self
        def to_ptr
            self.class.to_ptr.wrap(@ptr.to_ptr)
        end
	
	alias :__to_s__ :to_s
	def to_s
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

        # get the Ruby::DL equivalent for self
        def to_dlptr; @ptr end

	def is_a?(typename); self.class.is_a?(typename) end

        def inspect
            sprintf("<%s @%s (%s)>", self.class, to_dlptr.inspect, self.class.name)
        end
    end

    # Base class for compound types (structs, unions)
    class CompoundType < Type
        @writable = false
	# Initializes this object to the pointer +ptr+, and initializes it
	# to +init+. Valid values for +init+ are:
	# * a hash, in which case it is a { field_name => field_value } hash
	# * an array, in which case the fields are initialized in order
	# Note that a compound should be either fully initialized or not initialized
        def initialize(ptr, *init)
            super(ptr)
            return if init.empty?

            init = init.first if init.size == 1 && Hash === init.first

	    fields = self.class.fields
	    if fields.size != init.size
		raise ArgumentError, "wrong number of arguments (#{init.size} for #{fields.size})"
	    end
	    
            # init is either an array of values (the fields in order) or a hash
            # (named parameters)
            if Hash === init
                # init.each shall yield (name, value)
                init.each do |name, value| 
		    name = name.to_s
		    self[name] = value 
		end
            else
                # we assume that the values are given in order
                init.each_with_index do |value, idx| 
		    self[self.class.fields[idx].first] = value
		end
            end
        end

        class << self
            def new(*init);         __real_new__(nil, *init) end
            def wrap(ptr, *init);   __real_new__(ptr, *init) end

	    # Check if this type can be used in place of +typename+
	    # In case of compound types, we check that either self, or
	    # the first element field is +typename+
	    def is_a?(typename)
		if typename.respond_to?(:to_str) || Regexp === typename
		    typename === self.name || self.fields[0].last.is_a?(typename)
		else
		    super
		end
	    end

	    # Called by the extension to initialize the subclass
	    # For each field, it creates getters and setters on 
	    # the object, and a getter in the singleton class 
	    # which returns the field type
            def subclass_initialize
                @fields = get_fields.map! do |name, offset, type|
                    if !method_defined?(name)
			define_method(name) { get_field(name) }
                        if type.writable? || type < CompoundType
                            define_method("#{name}=") { |value| self[name] = value }
                        end
                    end
                    if !singleton_class.method_defined?(name)
                        singleton_class.send(:define_method, name) { || type }
                    end

                    [name, type]
                end
            end

	    # The list of fields
            attr_reader :fields
	    # Returns the type of +name+
            def [](name); @fields.find { |n, t| n == name }.last end
	    # Iterates on all fields
            def each_field # :yield:name, type
		@fields.each { |field| yield(*field) } 
	    end

	    def pretty_print_common(pp) # :nodoc:
                pp.group(2, '{') do
		    pp.breakable
                    all_fields = enum_for(:each_field).
                        collect { |name, field| [name,field] }
                    
                    pp.seplist(all_fields) do |field|
			yield(*field)
                    end
                end
		pp.breakable
		pp.text '}'
	    end

            def pretty_print(pp) # :nodoc:
		super
		pp.text ' '
		pretty_print_common(pp) do |name, type|
		    pp.text name
		    pp.text ' <'
		    pp.nest(2) do
			pp.pp type
		    end
		    pp.text '>'
		end
            end
        end

	def pretty_print(pp) # :nodoc:
	    self.class.pretty_print_common(pp) do |name, type|
		pp.text name
		pp.text "="
		pp.pp self[name]
	    end
	end

	# Sets the value of the field +name+. If +value+
	# is a hash, we expect that the field is a
	# compound type and initialize it using the
	# keys of +value+ as field names
        def []=(name, value)
	    if Hash === value
		attribute = get_field(name)
		value.each { |k, v| attribute[k] = v }
	    else
		set_field(name.to_s, value) 
	    end

	rescue ArgumentError => e
	    raise e, "no field #{name} in #{self.class.name}"
	rescue TypeError => e
	    raise e, "#{e.message} for #{self.class.name}.#{name}"
	end
	# Returns the value of the field +name+
        def [](name); get_field(name) end
        def to_ruby; self end
    end

    class IndirectType < Type
        @writable = false
    end

    # Base class for all arrays
    class ArrayType < IndirectType
        @writable = false

	def pretty_print(pp) # :nodoc:
	    all_fields = enum_for(:each_with_index).to_a

	    pp.text '['
	    pp.nest(2) do
		pp.breakable
		pp.seplist(all_fields) do |element|
		    element, index = *element 
		    pp.text "[#{index}] = "
		    pp.pp element
		end
	    end
	    pp.breakable
	    pp.text ']'
	end

        def to_ruby
	    if respond_to?(:to_str); to_str
	    else self 
	    end
	end
        include Enumerable
    end

    # Base class for all pointers
    class PointerType < IndirectType
        @writable = false

	# Returns 
	# * nil if this is a NULL pointer, and a string
	# * a String object if it is a pointer to a string. 
	# * self otherwise
        def to_ruby
	    if self.null?; nil
	    elsif respond_to?(:to_str); to_str
	    else self
	    end
       	end
    end

    # Base class for all enumeration types. Enumerations
    # are mappings from strings to integers
    class EnumType < Type
        class << self
	    # a value => key hash for each enumeration values
            attr_reader :values

            def pretty_print(pp) # :nodoc:
                super
		pp.text '{'
                pp.nest(2) do
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

    class Registry
        TYPE_BY_EXT = {
            ".c" => "c",
            ".cc" => "c",
            ".cxx" => "c",
            ".h" => "c",
            ".hh" => "c",
            ".hxx" => "c",
            ".tlb" => "tlb"
        }
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
            option_hash.to_a.collect { |opt| [ opt[0].to_s, opt[1] ] }
        end

	# Imports the +file+ into this registry. +kind+
	# is the file format or nil, in which case the
	# file format is guessed by extension (see TYPE_BY_EXT)
	# 
	# +options+ is an option hash. The Ruby bindings define
	# the following specific options:
	# * merge: merges +file+ into this repository. If this is
	#	   false, an exception is raised if +file+ contains
	#	   types already defined in +self+, event if the
	#	   definitions are the same.
	#
	# See Typelib documentation for other valid options 
	# (which are specific to the filetype).
        def import(file, kind = nil, options = {})
            kind    = Registry.guess_type(file) if !kind
	    do_merge = options.delete(:merge) || options.delete('merge')
            options = Registry.format_options(options)

            do_import(file, kind, do_merge, options)
        end
    end

    # Loads a library object for +lib_path+ and
    # initializes its type registry to +registry+.
    # If +registry+ is nil, a new registry is created
    def self.dlopen(lib_path, registry = nil)
        lib = DL.dlopen(lib_path)
        registry = Registry.new if !registry
        Library.new(lib, registry)
    end

    # A C library
    class Library < DelegateClass(DL::Handle)
	# The type registry for this library
        attr_reader :registry

	# Creates a new library wrapper
	# +lib+ is a DL::Handle object. +registry+
	# is the type registry
        def initialize(lib, registry)
            super(lib)
            @registry = registry
        end

        # Wraps a function defined in +self+
        #
        # +return_spec+ defines what the function returns. It can be either
        # an object or an array
        #   - if it is an object, then this object is the name of the type returned
        #     by the function, or nil if the function returns nothing. If +return_spec+
        #     is an array, the first element of this array defines this return type
        #   - the other elements of the array are *positional parameters*. They define
        #     what arguments of the function are either return values or both
        #     argument and return value.
        #     Use positive indexes to use an argument as a return value only and 
        #     negative indexes to use an argument as both input and output. Arguments
        #     are indexed starting from 1
        #
        # The wrapped function will behave as:
        #   output_values = wrapper.call(*input_values)
        #   
        # where output_values is the array of values specified by return_spec and
        # input_values the function arguments /without the arguments that are out-only
        # values/
        # 
        def wrap(name, return_spec = nil, *arg_types)
            if arg_types.include?(nil)
                raise ArgumentError, '"nil" is only allowed as return type'
            end
            return_spec = Array[*return_spec]
            return_type = return_spec.shift
            return_spec = return_spec.sort # MUST be sorted for #function_call to work properly

            return_type, *arg_types = Array[return_type, *arg_types].map do |typedef|
                next if typedef.nil?

                typedef = if typedef.respond_to?(:to_str)
                              registry.build(typedef.to_str)
                          elsif Class === typedef && typedef < Type
                              typedef
                          else
                              raise ArgumentError, "expected a Typelib::Type or a string, got #{typedef.inspect}"
                          end

                if typedef < CompoundType
                    raise ArgumentError, "Structs aren't allowed directly in a function call, use pointers instead"
                end

                typedef
            end
            return_type = nil if return_type && return_type.null?

            return_spec.each do |index|
                ary_idx = index.abs - 1
                if index == 0 || ary_idx >= arg_types.size
                    raise ArgumentError, "Index out of bound: there is no positional parameter #{index.abs}"
                elsif !(arg_types[ary_idx] < IndirectType)
                    raise ArgumentError, "Parameter #{index.abs} is supposed to be an output value, but it is not a pointer (#{arg_types[ary_idx].name})"
                end
            end

            dl_wrapper = do_wrap(name, return_type, *arg_types)
            return WrappedFunction.new(dl_wrapper, return_type, return_spec, arg_types)
        end
    end

    # A function wrapper
    class WrappedFunction
	attr_reader :spec
	def initialize(*spec); @spec = spec end
	# Calls the function with +args+ for arguments
	def call(*args); Typelib.function_call(args, *spec) end
	alias :[] :call
	# Checks that +args+ is a valid set of arguments for this function
	def filter(*args); Typelib.filter_function_args(args, *spec) end
	alias :check :filter
    end

    # Implement Typelib's argument handling
    # This method filters a particular argument given the user-supplied value and 
    # the argument expected type. It raises TypeError if +arg+ is of the wrong type
    def self.filter_argument(arg, expected_type) # :nodoc:
	if !(expected_type < IndirectType) && !(Kernel.immediate?(arg) || Kernel.numeric?(arg))
	    raise TypeError, "#{arg.inspect} cannot be used for #{expected_type.name} arguments"
	end

	filtered =  if expected_type < IndirectType
			if DL::PtrData === arg
			    arg
			elsif Type === arg
			    filter_value_arg(arg, expected_type)
			elsif expected_type.deference.name == "/char" && arg.respond_to?(:to_str)
			    # Ruby strings ARE null-terminated
			    # The thing which is not checked here is that there is no NULL bytes
			    # inside the string.
			    arg.to_str.to_ptr
			end
		    end

	if !filtered && (Kernel.immediate?(arg) || Kernel.numeric?(arg))
	    filtered = filter_numeric_arg(arg, expected_type)
	end

	if !filtered
	    raise TypeError, "cannot use #{arg.inspect} for a #{expected_type.name} argument"
	end
	filtered
    end

    # Creates an array of objects that can safely be passed to DL function call mechanism
    def self.filter_function_args(args, wrapper, return_type, return_spec, arg_types) # :nodoc:
        user_args_count = args.size
        
        # Create a Value object to collect the data we'll get
        # from output-only arguments
        return_spec.each do |index|
            next unless index > 0
            ary_idx = index - 1

	    type = arg_types[ary_idx]
	    buffer = if type < ArrayType then type.new
		     elsif type < PointerType then type.deference.new
		     else
			 raise ArgumentError, "unexpected output type #{type.name}"
		     end

            args.insert(ary_idx, buffer) # works because return_spec is sorted
        end

        # Check we have the right count of arguments
        if args.size != arg_types.size
            wrapper_args_count = args.size - user_args_count
            raise ArgumentError, "#{arg_types.size - wrapper_args_count} arguments expected, got #{user_args_count}"
        end

	args.each_with_index do |arg, idx|
	    args[idx] = filter_argument(arg, arg_types[idx])
	end
    end

    # Do the function call, handling return types
    def self.function_call(args, wrapper, return_type, return_spec, arg_types) # :nodoc:
	# Keep track of String objects for return values
	strings = return_spec.inject([]) do |strings, index|
	    ary_idx = index.abs - 1
	    strings[ary_idx] = true if String === args[ary_idx]
	    strings
	end
	
	filtered_args = filter_function_args(args, wrapper, return_type, return_spec, arg_types)

        # Do call the wrapper
        retval, retargs = do_call_function(wrapper, filtered_args, return_type, arg_types) 
        return if return_type.nil? && return_spec.empty?

        # Get the return array
        ruby_returns = []
        if return_type
            if DL::PtrData === retval
                ruby_returns << return_type.wrap(retval.to_ptr)
            else
                ruby_returns << retval
            end
        end

        return_spec.each do |index|
            ary_idx = index.abs - 1

            value = retargs[ary_idx]
            type  = arg_types[ary_idx]

	    if type < ArrayType
		ruby_returns << type.wrap(value)
	    else
		ruby_returns << type.deference.wrap(value).to_ruby
	    end
        end

        return *ruby_returns
    end
end

require 'dl'
require 'typelib_ruby'

