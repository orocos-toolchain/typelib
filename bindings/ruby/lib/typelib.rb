require 'enumerator'
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
            attr_reader :registry
            def writable?
                if !defined?(@writable)
                    superclass.writable?
                else
                    @writable
                end
            end
            def to_ptr; registry.build(name + "*") end
            def pretty_print(pp); pp.text name end
        end

        def ==(other)
            if Type === other
                self.class == other.class && to_dlptr == other.to_dlptr 
            else
                other == self.to_ruby
            end
        end

        def to_ptr
            self.class.to_ptr.new(@ptr.to_ptr)
        end

        # Get a pointer on this value
        def to_dlptr; @ptr end

        def inspect
            sprintf("<%s @%s (%s)>", self.class, to_dlptr.inspect, self.class.name)
        end
    end

    class CompoundType < Type
        @writable = false

        # The extension defines a @fields array of [ name, offset, type ] arrays
        # They define the list of available fields defined by this type
        class << self
            def each_field(&iter)
                @fields.each(&iter)
            end

            def subclass_initialize
                @fields = Array.new
                singleton_class = class << self; self end

                get_fields.each do |name, offset, type|
                    @fields << [name, type]

                    if !instance_methods.include?(name)
                        define_method(name, &lambda { || get_field(name) })
                        if type.writable?
                            define_method("#{name}=", &lambda { |value| set_field(name, value) })
                        end
                    end
                    if !singleton_class.instance_methods.include?(name)
                        singleton_class.send(:define_method, name, &lambda { || type })
                    end
                end
            end

            def [](name)
                @fields.find { |n, t| n == name }.last
                    
            end

            def pretty_print(pp)
                super

                pp.breakable
                pp.text '{'
                pp.breakable
                pp.group(2, '', '') do
                    all_fields = enum_for(:each_field).
                        collect { |name, field| [name,field] }.
                        sort_by { |name, _| name.downcase }
                    
                    pp.breakable
                    pp.seplist(all_fields) do |field|
                        name, type = *field
                        pp.text name
                        pp.group(2, '<', '>') do
                            type.pretty_print(pp)
                        end
                    end
                end
                pp.breakable
                pp.text '}'
            end
        end

        def []=(name, value); set_field(name, value) end
        def [](name); get_field(name) end
        def to_ruby; self end

    end

    class ArrayType < Type
        @writable = false

        def to_ruby; self end
        include Enumerable
    end

    class PointerType < Type
        @writable = false
        def to_ruby; self end
    end
    class EnumType < Type
        class << self
            attr_reader :values
            def pretty_print(pp)
                super
                pp.group(2, '{', '}') do
                    pp.seplist(keys) do |keydef|
                        pp.text keydef.join(" = ")
                    end
                end
            end
        end
    end

    class Registry
        TYPE_BY_EXT = {
            "c" => "c",
            "cc" => "c",
            "cxx" => "c",
            "h" => "c",
            "hh" => "c",
            "hxx" => "c",
            "tlb" => "tlb"
        }
        def self.guess_type(file)
            ext = File.extname(file)[1..-1]
            if TYPE_BY_EXT.has_key?(ext)
                return TYPE_BY_EXT[ext]
            else
                raise "Cannot guess file type for #{file}: unknown extension '#{ext}'"
            end
        end

        def self.format_options(option_hash)
            return nil if !option_hash
            option_hash.to_a.collect { |opt| [ opt[0].to_s, opt[1] ] }
        end

        def initialize
            @wrappers = Hash.new
        end

        def import(file, kind = nil, options = nil)
            kind = Registry.guess_type(file) if !kind
            options = Registry.format_options(options)
            do_import(file, kind, options)
        end

        def get(name); @wrappers[name] || do_get(name) end
        def build(name); @wrappers[name] || do_build(name) end
    end

    def self.dlopen(lib_path, registry = nil)
        lib = DL.dlopen(lib_path)
        registry = Registry.new if !registry
        Library.new(lib, registry)
    end

    class Library < DelegateClass(DL::Handle)
        attr_reader :registry
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
        # input_values the function arguments /without the arguments that are output 
        # values/
        # 
        def wrap(name, return_spec = nil, *arg_types)
            if arg_types.include?(nil)
                raise ArgumentError, '"nil" is only allowed as return type'
            end
            return_spec = Array[*return_spec]
            return_type = return_spec.shift
            return_spec = return_spec.sort # MUST be sorted for #insert to work (see on top of #function_call

            return_type, *arg_types = Array[return_type, *arg_types].collect do |typedef|
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
                elsif !(arg_types[ary_idx] < PointerType)
                    raise ArgumentError, "Parameter #{index.abs} is supposed to be an output value, but it is not a pointer (#{arg_types[ary_idx].name})"
                end
            end

            dl_wrapper = do_wrap(name, return_type, *arg_types)
            return lambda do |*args|
                Typelib.function_call(dl_wrapper, args, return_type, return_spec, arg_types)
            end
        end
    end

    def self.function_call(wrapper, args, return_type, return_spec, arg_types)
        user_args_count = args.size
        
        # Create a Value object to collect the data we'll get
        # from output-only arguments
        return_spec.each do |index|
            next unless index > 0
            ary_idx = index - 1
            args.insert(ary_idx, arg_types[ary_idx].deference.new(nil))
        end

        # Check we have the right count of arguments
        if args.size != arg_types.size
            wrapper_args_count = args.size - user_args_count
            raise ArgumentError, "#{arg_types.size - wrapper_args_count} arguments expected, got #{user_args_count}"
        end

        # The only things we can handle are:
        #  - Value objects
        #  - String objects
        #  - immediate values
        filtered_args = []
        args.each_with_index do |arg, idx|
            expected_type = arg_types[idx]
            if !(expected_type < PointerType) && !Kernel.immediate?(arg)
                raise TypeError, "#{arg.inspect} cannot be used for #{expected_type.name} arguments"
            end

            filtered =  if DL::PtrData === arg
                            arg
                        elsif Kernel.immediate?(arg)
                            filter_immediate_arg(arg, expected_type)
                        elsif Type === arg
                            filter_value_arg(arg, expected_type)
                        elsif expected_type.deference.name == "/char" && arg.respond_to?(:to_str)
                            # Ruby strings ARE null-terminated
                            # The thing which is not checked here is that there is no NULL bytes
                            # inside of the string.
                            arg.to_str.to_ptr
                        end

            if !filtered
                raise TypeError, "cannot use #{arg.inspect} for a #{expected_type.name} argument"
            end
            filtered_args << filtered
        end

        # Do call the wrapper
        # The C part will take care of immediate values that are also output values
        retval, retargs = do_call_function(wrapper, filtered_args, return_type, arg_types) 
        return if return_type.nil? && return_spec.empty?

        # Get the return array
        ruby_returns = []
        if return_type
            if DL::PtrData === retval
                ruby_returns << return_type.new(retval.to_ptr)
            else
                ruby_returns << retval
            end
        end

        return_spec.each do |index|
            ary_idx = index.abs - 1

            value = retargs[ary_idx]
            type  = arg_types[ary_idx]
            ruby_returns << type.deference.new(value).to_ruby
        end

        return *ruby_returns
    end
end

require 'dl'
require 'typelib_api'

