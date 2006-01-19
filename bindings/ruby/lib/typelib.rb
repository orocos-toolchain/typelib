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

        return_type, *arg_types = Array[return_type, *arg_types].collect do |typedef|
            next if typedef.nil?

            typedef = if Type === typedef
                          typedef
                      elsif typedef.respond_to?(:to_str)
                          lib.registry.build(typedef.to_str)
                      else
                          raise ArgumentError, "expected a Typelib::Type or a string, got #{typedef.inspect}"
                      end

            if typedef.compound?
                raise ArgumentError, "Structs aren't allowed directly in a function call, use pointers instead"
            end

            typedef
        end

        return_spec.each do |index|
            ary_idx = index.abs - 1
            if index == 0 || ary_idx >= arg_types.size
                raise ArgumentError, "Index out of bound: there is no positional parameter #{index.abs}"
            elsif !arg_types[ary_idx].pointer?
                raise ArgumentError, "Parameter #{index.abs} is supposed to be an output value, but it is not a pointer"
            end
        end

        dl_wrapper = do_wrap(lib, name, return_type, *arg_types)

        return lambda do |*args| 
            user_args_count = args.size

            return_spec.each do |index|
                next unless index > 0
                args.insert(index - 1, Value.new(nil, arg_types[index - 1].deference))
            end

            if args.size != arg_types.size
                wrapper_args_count = args.size - user_args_count
                raise ArgumentError, "#{arg_types.size - wrapper_args_count} arguments expected, got #{user_args_count}"
            end

            ret = call_function(return_type, args, arg_types, dl_wrapper) 
            if return_type.nil? && return_spec.empty?
                return
            end

            retval  = ret.shift
            retargs = ret.shift

            ruby_returns = []
            if return_type
                ruby_returns << 
                    if DL::PtrData === retval
                        Value.new(retval, return_type.deference).to_ruby
                    else
                        retval
                    end
            end

            return_spec.each do |index|
                ary_idx = index.abs - 1
                ruby_returns << Value.new(retargs[ary_idx], arg_types[ary_idx].deference).to_ruby
            end

            if ruby_returns.size > 1
                ruby_returns
            else
                ruby_returns.first
            end
        end
    end
end

require 'dl'
require 'typelib_api'

