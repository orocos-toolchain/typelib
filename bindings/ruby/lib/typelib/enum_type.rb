module Typelib
    # Base class for all enumeration types. Enumerations
    # are mappings from strings to integers
    #
    # See the Typelib module documentation for an overview about how types are
    # values are represented.
    class EnumType < Type
        def self.from_ruby(value)
            v = new
            v.typelib_from_ruby(value)
            v
        end

        class << self
	    # A value => key hash for each enumeration values
            attr_reader :values

            def pretty_print(pp, verbose = false) # :nodoc:
                super
		pp.text '{'
                pp.nest(2) do
                    keys = self.keys.sort_by(&:last)
		    pp.breakable
                    pp.seplist(keys) do |keydef|
                        if verbose
                            pp.text keydef.join(" = ")
                        else
                            pp.text keydef[0]
                        end
                    end
                end
		pp.breakable
		pp.text '}'
            end
        end
    end
end
