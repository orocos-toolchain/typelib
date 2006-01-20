require 'delegate'
require 'pp'
module DL
    class Handle
        def registry; @typelib_registry end
    end
end

module Typelib
    class Value
        # Get a DL::Ptr object for this value
        def to_ptr; @ptr end

        def respond_to?(name)
            name = name.to_s
            value = if name[-1] == ?=
                        field_attributes(name[0..-2]) == 1
                    else
                        field_attributes(name)
                    end

            if value.nil?
                super
            else
                value
            end
        end
        def method_missing(name, *args, &proc)
            name = name.to_s
            value = if name[-1] == ?=
                        set_field(name[0..-2], args[0])
                    elsif args.empty?
                        get_field(name)
                    end

            value || super
        end
    end

    class Type
        attr_reader :registry
        def respond_to?(name); has_field?(name.to_s) end
        def method_missing(name, *args, &proc)
            begin
                get_field(name.to_s)
            rescue NoMethodError
                super
            end
        end

        def to_ptr
            registry.build(name + "*")
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

        def import(file, kind = nil, options = nil)
            kind = Registry.guess_type(file) if !kind
            options = Registry.format_options(options)
            do_import(file, kind, options)
        end
    end

    class ValueArray < Value
        def initialize(ptr, type)
            if !type.array?
                raise ArgumentError, "Cannot build a Typelib::ValueArray object with a type which is not an array"
            elsif !ptr
                raise ArgumentError, "Cannot build a Typelib::ValueArray object without a pointer"
            end

            super
        end

        include Enumerable
    end

    def self.dlopen(lib_path, registry)
        lib = DL.dlopen(lib_path)
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

            return_type, *arg_types = Array[return_type, *arg_types].collect do |typedef|
                next if typedef.nil?

                typedef = if Type === typedef
                              typedef
                          elsif typedef.respond_to?(:to_str)
                              registry.build(typedef.to_str)
                          else
                              raise ArgumentError, "expected a Typelib::Type or a string, got #{typedef.inspect}"
                          end

                if typedef.compound?
                    raise ArgumentError, "Structs aren't allowed directly in a function call, use pointers instead"
                end

                typedef
            end
            return_type = nil if return_type && return_type.null?

            return_spec.each do |index|
                ary_idx = index.abs - 1
                if index == 0 || ary_idx >= arg_types.size
                    raise ArgumentError, "Index out of bound: there is no positional parameter #{index.abs}"
                elsif !arg_types[ary_idx].pointer?
                    raise ArgumentError, "Parameter #{index.abs} is supposed to be an output value, but it is not a pointer"
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
            args.insert(ary_idx, Value.new(nil, arg_types[ary_idx].deference))
        end

        # The only things we can handle are:
        #  - Value objects
        #  - String objects
        #  - immediate values
        filtered_args = []
        args.each_with_index do |arg, idx|
            if Value === arg || DL::PtrData === arg || Kernel.immediate?(arg)
                filtered_args << arg
            else
                # Check that idx is an indirect type on char, and that arg is a string
                expected_type = arg_types[idx]
                if expected_type.deference.name != "/char" || !arg.respond_to?(:to_str)
                    raise TypeError, "cannot cast #{arg} on #{expected_type.deference.name}"
                end

                # Ruby strings ARE null-terminated
                # The thing which is not checked here is that there is no NULL bytes
                # inside of the string. Frankly, I don't care
                filtered_args << arg.to_str.to_ptr
            end
        end

        # Check we had the right count of arguments
        if args.size != arg_types.size
            wrapper_args_count = args.size - user_args_count
            raise ArgumentError, "#{arg_types.size - wrapper_args_count} arguments expected, got #{user_args_count}"
        end

        # Do call the wrapper
        # The C part will take care of immediate values that are also output values
        retval, retargs = do_call_function(wrapper, args, return_type, arg_types) 
        return if return_type.nil? && return_spec.empty?

        # Get the return array
        ruby_returns = []
        if return_type
            # There's no point of returning a simple value by pointer,
            # so we assume that if the function returns a pointer on a simple
            # type, that's what the user wants
            if DL::PtrData === retval && !return_type.deference.null?
                ruby_value = Value.new(retval, return_type.deference).to_ruby

                if Kernel.immediate?(ruby_value)
                    ruby_returns << Value.new(retval.to_ptr, return_type)
                else
                    ruby_returns << ruby_value
                end
            else
                ruby_returns << retval
            end
        end

        return_spec.each do |index|
            ary_idx = index.abs - 1

            value = retargs[ary_idx]
            type  = arg_types[ary_idx]
            ruby_returns << Value.new(value, type.deference).to_ruby
        end

        return *ruby_returns
    end
end

require 'dl'
require 'typelib_api'

