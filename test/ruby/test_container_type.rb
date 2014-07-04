require 'typelib'
require 'minitest/spec'

describe Typelib::ArrayType do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "the type model" do
        describe "#to_h" do
            attr_reader :container_t, :element_t
            before do
                @container_t = registry.create_container '/std/vector', '/int32_t'
                @element_t = container_t.deference
            end

            it "should be able to describe the type" do
                expected = Hash['class' => 'Typelib::ContainerType',
                                'name' => container_t.name,
                                'element' => element_t.to_h_minimal(layout_info: false)]
                assert_equal expected, container_t.to_h(layout_info: false, recursive: false)
            end

            it "should describe the sub-type fully if recursive is true" do
                expected = Hash['class' => 'Typelib::ContainerType',
                                'name' => container_t.name,
                                'element' => element_t.to_h(layout_info: false)]
                assert_equal expected, container_t.to_h(layout_info: false, recursive: true)
            end
        end
    end
end

