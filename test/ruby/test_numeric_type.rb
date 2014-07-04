require 'typelib'
require 'minitest/spec'

describe Typelib::NumericType do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "the type model" do
        describe "#to_h" do
            attr_reader :int_t, :uint_t, :float_t
            before do
                @int_t = registry.get '/int32_t'
                @uint_t = registry.get '/uint32_t'
                @float_t = registry.get '/float'
            end
            it "should report a class of 'NumericType'" do
                assert_equal 'Typelib::NumericType', int_t.to_h['class']
            end
            it "should always have a size field" do
                info = int_t.to_h(layout_info: false)
                assert_equal 4, info['size']
            end
            it "should add the unsigned flag to integer type descriptions" do
                assert_equal false, int_t.to_h['unsigned']
                assert_equal true, uint_t.to_h['unsigned']
            end
            it "should not add the unsigned flag to float type descriptions" do
                assert !float_t.to_h.has_key?('unsigned')
            end
            it "should convert an integer type description" do
                expected = Hash['name' => int_t.name,
                                'class' => 'Typelib::NumericType',
                                'size' => 4,
                                'integer' => true,
                                'unsigned' => false]
                assert_equal expected, int_t.to_h
            end
            it "should convert a float type description" do
                expected = Hash['name' => float_t.name,
                                'class' => 'Typelib::NumericType',
                                'size' => 4,
                                'integer' => false]
                assert_equal expected, float_t.to_h
            end
        end
    end

end

