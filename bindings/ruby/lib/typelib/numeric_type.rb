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
    end
end
