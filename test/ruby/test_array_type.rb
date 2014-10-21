require 'typelib/test'

describe Typelib::ArrayType do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "the type model" do
        describe "#to_h" do
            attr_reader :array_t, :element_t
            before do
                @array_t = registry.build '/int32_t[10]'
                @element_t = array_t.deference
            end

            it "should be able to describe the type" do
                expected = Hash[class: 'Typelib::ArrayType',
                                name: array_t.name,
                                element: element_t.to_h_minimal(layout_info: false),
                                length: 10]
                assert_equal expected, array_t.to_h(layout_info: false, recursive: false)
            end

            it "should describe the sub-type fully if recursive is true" do
                expected = Hash[class: 'Typelib::ArrayType',
                                name: array_t.name,
                                element: element_t.to_h(layout_info: false),
                                length: 10]
                assert_equal expected, array_t.to_h(layout_info: false, recursive: true)
            end
        end
    end

    describe "#to_simple_value" do
        attr_reader :ruby_array, :array
        before do
            array_t = registry.build '/int32_t[10]'
            element_t = array_t.deference
            @ruby_array = (1..10).to_a
            @array = Typelib.from_ruby(ruby_array, array_t)
        end

        it "returns the corresponding plain ruby array if :pack_simple_arrays is false" do
            value = array.to_simple_value(pack_simple_arrays: false)
            assert_equal ruby_array, value
        end
        it "returns the pack code as well as the packed data if :pack_simple_arrays is true and the array elements are numerics" do
            value = array.to_simple_value(pack_simple_arrays: true)
            assert_equal ruby_array, Base64.strict_decode64(value[:data]).unpack("#{value[:pack_code]}*")
        end
    end
end

