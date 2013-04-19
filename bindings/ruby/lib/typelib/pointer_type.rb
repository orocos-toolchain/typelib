module Typelib
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
end
