require 'typelib'
require 'minitest/spec'

describe Typelib::CompoundType do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "the type model" do
        describe "#to_h" do
            attr_reader :compound_t, :f0_t, :f1_t
            before do
                f0_t = @f0_t = registry.get '/int32_t'
                f1_t = @f1_t = registry.get '/float'
                @compound_t = registry.create_compound '/C' do |builder|
                    builder.add 'f0', f0_t, 0
                    builder.add 'f1', f1_t, 100
                end
            end

            it "should be able to describe the type" do
                expected = Hash['class' => 'Typelib::CompoundType',
                                'name' => compound_t.name,
                                'fields' => [
                                    Hash['name' => 'f0', 'type' => f0_t.to_h_minimal(layout_info: false)],
                                    Hash['name' => 'f1', 'type' => f1_t.to_h_minimal(layout_info: false)]
                                ]]
                assert_equal expected, compound_t.to_h(layout_info: false, recursive: false)
            end

            it "should describe the sub-type fully if recursive is true" do
                expected = Hash['class' => 'Typelib::CompoundType',
                                'name' => compound_t.name,
                                'fields' => [
                                    Hash['name' => 'f0', 'type' => f0_t.to_h(layout_info: false)],
                                    Hash['name' => 'f1', 'type' => f1_t.to_h(layout_info: false)]
                                ]]
                assert_equal expected, compound_t.to_h(layout_info: false, recursive: true)
            end

            it "should add the field offsets if layout_info is true" do
                expected = Hash['class' => 'Typelib::CompoundType',
                                'name' => compound_t.name,
                                'size' => compound_t.size,
                                'fields' => [
                                    Hash['name' => 'f0', 'type' => f0_t.to_h_minimal(layout_info: true), 'offset' => 0],
                                    Hash['name' => 'f1', 'type' => f1_t.to_h_minimal(layout_info: true), 'offset' => 100]
                                ]]
                assert_equal expected, compound_t.to_h(layout_info: true, recursive: false)
            end
        end
    end

    describe "#to_simple_value" do
        it "returns a hash of field-to-value mappings" do
            compound_t = registry.create_compound '/C' do |builder|
                builder.add 'f0', '/int32_t', 0
            end
            compound = compound_t.new
            compound.f0 = 10
            assert_equal Hash['f0' => 10], compound.to_simple_value
        end
    end
end


