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

    class Array < Value
        def initialize(ptr, type)
            if !type.array?
                raise ArgumentError, "Cannot build a Typelib::Array object with a type which is not an array"
            elsif !ptr
                raise ArgumentError, "Cannot build a Typelib::Array object without a pointer"
            end

            super
        end
    end
end

require 'dl'
require 'typelib_api'


