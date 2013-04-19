module Typelib
    # Convenience registry class that adds the builtin C++ types at construction
    # time
    class CXXRegistry < Registry
        def initialize
            super
            Registry.add_standard_cxx_types(self)
        end
    end
end
