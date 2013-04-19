module Typelib
    # Get the name for 'char'
    reg = Registry.new(false)
    Registry.add_standard_cxx_types(reg)
    CHAR_T = reg.get('/char')

    convert_from_ruby TrueClass, '/bool' do |value, typelib_type|
        Typelib.from_ruby(1, typelib_type)
    end
    convert_from_ruby FalseClass, '/bool' do |value, typelib_type|
        Typelib.from_ruby(0, typelib_type)
    end
    convert_to_ruby '/bool' do |value|
        value != 0
    end

    convert_from_ruby String, '/std/string' do |value, typelib_type|
        typelib_type.wrap([value.length, value].pack("QA#{value.length}"))
    end
    convert_to_ruby '/std/string', String do |value|
        value.to_byte_array[8..-1]
    end
    specialize '/std/string' do
        def to_str
            Typelib.to_ruby(self)
        end

        def concat(other_string)
            if other_string.respond_to?(:to_str)
                super(Typelib.from_ruby(other_string, self.class))
            else super
            end
        end
    end
    specialize '/std/vector<>' do
        include Typelib::ContainerType::StdVector
    end

    ####
    # C string handling
    if String.instance_methods.include? :ord
        convert_from_ruby String, CHAR_T.name do |value, typelib_type|
            if value.size != 1
                raise ArgumentError, "trying to convert '#{value}', a string of length different than one, to a character"
            end
            Typelib.from_ruby(value[0].ord, typelib_type)
        end
    else
        convert_from_ruby String, CHAR_T.name do |value, typelib_type|
            if value.size != 1
                raise ArgumentError, "trying to convert '#{value}', a string of length different than one, to a character"
            end
            Typelib.from_ruby(value[0], typelib_type)
        end
    end
    convert_from_ruby String, "#{CHAR_T.name}[]" do |value, typelib_type|
        result = typelib_type.new
        Type::from_string(result, value, true)
        result
    end
    convert_to_ruby "#{CHAR_T.name}[]", String do |value|
        Type::to_string(value, true)
    end
    specialize "#{CHAR_T.name}[]" do
        def to_str
            Type::to_string(self, true)
        end
    end
    convert_from_ruby String, "#{CHAR_T.name}*" do |value, typelib_type|
        result = typelib_type.new
        Type::from_string(result, value, true)
        result
    end
    convert_to_ruby "#{CHAR_T.name}*", String do |value|
        Type::to_string(value, true)
    end
    specialize "#{CHAR_T.name}*" do
        def to_str
            Type::to_string(self, true)
        end
    end
end


