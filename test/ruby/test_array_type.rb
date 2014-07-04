require 'typelib'
require 'minitest/spec'

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
                expected = Hash['class' => 'Typelib::ArrayType',
                                'name' => array_t.name,
                                'element' => element_t.to_h_minimal(layout_info: false),
                                'length' => 10]
                assert_equal expected, array_t.to_h(layout_info: false, recursive: false)
            end

            it "should describe the sub-type fully if recursive is true" do
                expected = Hash['class' => 'Typelib::ArrayType',
                                'name' => array_t.name,
                                'element' => element_t.to_h(layout_info: false),
                                'length' => 10]
                assert_equal expected, array_t.to_h(layout_info: false, recursive: true)
            end
        end
    end
end

