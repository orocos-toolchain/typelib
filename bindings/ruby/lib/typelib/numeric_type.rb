module Typelib
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
        rescue TypeError => e
            raise Typelib::UnknownConversionRequested.new(value, self), "cannot convert #{value} (#{value.class}) to #{self}", e.backtrace
        end

        # Returns the description of a type using only simple ruby objects
        # (Hash, Array, Numeric and String).
        # 
        #    { 'name' => TypeName,
        #      'class' => 'EnumType',
        #      'integer' => Boolean,
        #      # Only for integral types
        #      'unsigned' => Boolean,
        #      # Unlike with the other types, the 'size' field is always present
        #      'size' => SizeOfTypeInBytes 
        #    }
        #
        # @option (see Type#to_h)
        # @return (see Type#to_h)
        def self.to_h(options = Hash.new)
            info = super
            info['size'] = size
            if integer?
                info['integer'] = true
                info['unsigned'] = unsigned?
            else
                info['integer'] = false
            end
            info
        end

        # Pack codes as [size, unsigned?, big_endian?] => code
        INTEGER_PACK_CODES = Hash[
            # a.k.a. /(u)int8_t
            [1, true, false]  => 'C',
            [1, true, true]   => 'C',
            [1, false, false] => 'c',
            [1, false, true]  => 'c',
            # a.k.a. /(u)int16_t
            [2, true, false]  => 'S<',
            [2, true, true]   => 'S>',
            [2, false, false] => 's<',
            [2, false, true]  => 's>',
            # a.k.a. /(u)int32_t
            [4, true, false]  => 'L<',
            [4, true, true]   => 'L>',
            [4, false, false] => 'l<',
            [4, false, true]  => 'l>',
            # a.k.a. /(u)int64_t
            [8, true, false]  => 'Q<',
            [8, true, true]   => 'Q>',
            [8, false, false] => 'q<',
            [8, false, true]  => 'q>']
        FLOAT_PACK_CODES = Hash[
            # a.k.a. /float
            [4, false]  => 'e',
            [4, true]   => 'g',
            # a.k.a. /double
            [8, false]  => 'E',
            [8, true]   => 'G']

        # Returns the Array#pack code that matches this type
        #
        # The endianness is the one of the local OS
        def self.pack_code
            if integer?
                INTEGER_PACK_CODES[[size, unsigned?, Typelib.big_endian?]]
            else
                FLOAT_PACK_CODES[[size, Typelib.big_endian?]]
            end
        end

        # (see Type#to_simple_value)
        #
        # Numerics are returned as their value (a number)
        def to_simple_value(options = Hash.new)
            to_ruby
        end
    end
end
