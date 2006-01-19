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
            ext = File.extname(file)
            if TYPE_BY_EXT.has_key?(ext)
                return TYPE_BY_EXT[ext]
            else
                raise "Cannot guess file type for #{file}"
            end
        end

        def self.format_options(option_hash)
            return nil if !option_hash
            option_hash.to_a
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

    # Wraps a function defined in +lib+
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
    def self.wrap(lib, name, return_spec = nil, *arg_types)
        if arg_types.include?(nil)
            raise ArgumentError, '"nil" is only allowed as return type'
        end
        return_spec = Array[*return_spec]
        return_type = return_spec.shift

        return_spec.each do |index|
            if index == 0 || index.abs >= arg_types.size
                raise ArgumentError, "Index out of bound: there is no positional parameter #{index.abs}"
            elsif !arg_types[index.abs].pointer?
                raise ArgumentError, "Parameter #{index.abs} is supposed to be an output value, but it is not a pointer"
            end
        end

        return_type, *arg_types = Array[return_type, *arg_types].collect do |typedef|
            next if typedef.nil?

            typedef = if Type === typedef
                          typedef
                      elsif typedef.respond_to?(:to_str)
                          lib.registry.build(typedef.to_str)
                      else
                          raise ArgumentError, "expected a Typelib::Type or a string, got #{typedef}"
                      end

            if typedef.compound?
                raise ArgumentError, "Structs aren't allowed directly in a function call, use pointers instead"
            end

            typedef
        end

        # Change enums into symbols. Pointers are handled in the Ruby code

        dl_wrapper = build_basic_wrapper(lib, name, return_type, return_spec, arg_types)

        return lambda do |*args| 
            return_spec.each do |index|
                next unless index > 0
                args.insert(index, Value.new(nil, arg_types[index]))
            end

            if args.size != arg_types.size
                raise ArgumentError, "#{arg_types.size - 1} arguments expected, got #{ruby_input.size}"
            end

            ret = call_function(return_type, args, arg_types, dl_wrapper) 
            if return_type.nil? && return_spec.empty?
                return
            end

            retval  = ret.shift
            retargs = ret.shift

            ruby_returns = []
            if return_type
                ruby_returns << if retval === DL::PtrData
                                    ruby_returns << make_return_value(retval, return_type.deference)
                                else
                                    retval
                                end
            end

            ruby_returns << retval if return_type
            return_spec.each do |index|
                ruby_returns << arg_types[index.abs].deference(retargs[index.abs])
            end
            ruby_returns
        end
    end

    def self.build_basic_wrapper(lib, name, return_type, return_spec, arg_types)
        ruby_output             = []
        ruby_needs_allocation   = []
        ruby_input              = []
            arg_type = arg_types[index.abs]
            ruby_output << arg_type
            ruby_input  << arg_type if index < 0
            ruby_needs_allocation << arg_type
        end
        
        do_wrap(lib, name, return_spec, *arg_types)
    end
end

require 'dl'
require 'typelib_api'

