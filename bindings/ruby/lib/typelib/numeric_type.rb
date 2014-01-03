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
        end
    end
end
