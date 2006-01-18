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

    def self.wrap(lib, name, return_type = nil, *arg_types)

        if arg_types.include?(nil)
            raise ArgumentError, '"nil" is only allowed as return type'
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
            elsif typedef.pointer? && !typedef.deference.compound?
                raise ArgumentError, "Passing non-structs as pointers is not supported yet"
            end

            typedef
        end
        
        dl_wrapper = do_wrap(lib, name, return_type, *arg_types)
        return lambda do |*args| 
            if args.size != arg_types.size
                raise ArgumentError, "#{arg_types.size - 1} arguments expected, got #{args.size}"
            end
            ret = call_function(return_type, args, arg_types, dl_wrapper) 
            
            # Create a Value object if needed
            if DL::PtrData === ret
                Value.new(ret, return_type.deference)
            else
                ret
            end
        end
    end
end

require 'dl'
require 'typelib_api'


