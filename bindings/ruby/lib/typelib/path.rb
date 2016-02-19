module Typelib
    # Path to a typelib value that is child of another one
    class Path
        # Call operation. Requires a method name (as a symbol) and a list of
        # arguments
        CALL    = :call
        # Iteration operator. Requiresd a method name (as a symbol) and a list
        # of arguments
        ITERATE = :iterate

        # Set of operations that should be performed to reach the target from
        # the root. This is a list of pairs [operation_code, spec] where spec is
        # an array containing the arguments required by the operation
        attr_reader :elements

        def initialize(elements)
            @elements   = elements.dup
        end

        def size
            elements.size
        end

        def initialize_copy(old)
            super
            @elements = old.elements.dup
        end

        def +(other)
            Path.new(elements + other.elements)
        end

        # Adds a call to the path
        def unshift_call(*m)
            elements.unshift([:call, m])
        end

        # Adds an iteration to the path
        def unshift_iterate(*m)
            elements.unshift([:iterate, m])
        end

        # Adds a call to the path
        def push_call(*m)
            elements.push([:call, m])
        end

        # Adds an iteration to the path
        def push_iterate(*m)
            elements.push([:iterate, m])
        end

        # Resolves all the values described by this path on +root+
        def resolve(root, elements = self.elements.dup)
            if elements.empty?
                return [root]
            end

            operation = elements.shift
            send(operation[0], operation[1], root, elements)
        end

        # Performs a call operation and recursively calls {resolve} on the
        # method's return value
        def call(spec, value, elements)
            if value = value.send(*spec)
                resolve(value, elements)
            else []
            end
        end

        # Performs an iteration operation and recursively calls {resolve} on the
        # elements
        def iterate(spec, value, elements)
            result = []
            value.send(*spec) do |el|
                result.concat(resolve(el, elements.dup))
            end
            result
        end
    end
end

