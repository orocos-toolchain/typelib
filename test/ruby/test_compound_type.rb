require 'typelib/test'

describe Typelib::CompoundType do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "plain objects" do
        before do
            @converted_t = registry.create_compound '/Converted' do |builder|
                builder.add 'f0', '/int'
                builder.add 'f1', '/double'
            end
            @converted_ruby_t = Class.new
            @converted_t.convert_from_ruby @converted_ruby_t do |*|
                @converted_t.new(f0: 10, f1: 20)
            end
            @converted_t.extend_for_custom_convertions
            @compound_t = registry.create_compound '/C' do |builder|
                builder.add 'f', @converted_t
            end
        end
        it "calls #initialize on .new" do
            @compound_t.class_eval do
                def initialize
                    super
                    self.f.f0 = 10
                end
            end
            assert_equal 10, @compound_t.new.f.f0
        end
        it "applies and removes the converted types used during initialization" do
            converted_ruby_t = @converted_ruby_t
            @compound_t.class_eval do
                define_method :initialize do
                    super()
                    self.f = converted_ruby_t.new
                end
            end
            t = @compound_t.new
            assert_equal 10, t.raw_get('f').f0
            assert_equal 20, t.raw_get('f').f1
            t.raw_set('f', @converted_t.new(f0: 0, f1: 0))
            t.apply_changes_from_converted_types
            assert_equal 0, t.raw_get('f').f0
            assert_equal 0, t.raw_get('f').f1
        end
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
                expected = Hash[class: 'Typelib::CompoundType',
                                name: compound_t.name,
                                fields: [
                                    Hash[name: 'f0', type: f0_t.to_h_minimal(layout_info: false)],
                                    Hash[name: 'f1', type: f1_t.to_h_minimal(layout_info: false)]
                                ]]
                assert_equal expected, compound_t.to_h(layout_info: false, recursive: false)
            end

            it "should describe the sub-type fully if recursive is true" do
                expected = Hash[class: 'Typelib::CompoundType',
                                name: compound_t.name,
                                fields: [
                                    Hash[name: 'f0', type: f0_t.to_h(layout_info: false)],
                                    Hash[name: 'f1', type: f1_t.to_h(layout_info: false)]
                                ]]
                assert_equal expected, compound_t.to_h(layout_info: false, recursive: true)
            end

            it "should add the field offsets if layout_info is true" do
                expected = Hash[class: 'Typelib::CompoundType',
                                name: compound_t.name,
                                size: compound_t.size,
                                fields: [
                                    Hash[name: 'f0', type: f0_t.to_h_minimal(layout_info: true), offset: 0],
                                    Hash[name: 'f1', type: f1_t.to_h_minimal(layout_info: true), offset: 100]
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


