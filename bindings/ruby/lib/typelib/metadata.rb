module Typelib
    # Class holding metadata information for types and compound fields
    class MetaData
        def each
            if !block_given?
                return enum_for(:each)
            end
            keys.each do |k|
                yield(k, get(k))
            end
        end
        def [](index)
            get(index)
        end
        def []=(index,value)
            set(index,value)
        end
        def set(key, *values)
            clear(key)
            add(key, *values)
        end

        def pretty_print(pp)
            pp.seplist(each.to_a) do |entry|
                key, values = *entry
                pp.text "#{key} ="
                pp.breakable
                pp.seplist(values) do |v|
                    v.pretty_print(pp)
                end
            end
        end
    end
end

