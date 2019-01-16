require 'typelib/test'

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
                assert_equal 'Typelib::NumericType', int_t.to_h[:class]
            end
            it "should always have a size field" do
                info = int_t.to_h(layout_info: false)
                assert_equal 4, info[:size]
            end
            it "should add the unsigned flag to integer type descriptions" do
                assert_equal false, int_t.to_h[:unsigned]
                assert_equal true, uint_t.to_h[:unsigned]
            end
            it "should not add the unsigned flag to float type descriptions" do
                assert !float_t.to_h.has_key?(:unsigned)
            end
            it "should convert an integer type description" do
                expected = Hash[name: int_t.name,
                                class: 'Typelib::NumericType',
                                size: 4,
                                integer: true,
                                unsigned: false]
                assert_equal expected, int_t.to_h
            end
            it "should convert a float type description" do
                expected = Hash[name: float_t.name,
                                class: 'Typelib::NumericType',
                                size: 4,
                                integer: false]
                assert_equal expected, float_t.to_h
            end
        end

        describe "#pack_code" do
            it "can return the pack code of a float" do
                float_t = registry.get '/double'
                if Typelib.big_endian?
                    assert_equal "G", float_t.pack_code
                else
                    assert_equal "E", float_t.pack_code
                end
            end
            it "can return the pack code of an integer" do
                int_t = registry.get '/int32_t'
                if Typelib.big_endian?
                    assert_equal "l>", int_t.pack_code
                else
                    assert_equal "l<", int_t.pack_code
                end
            end
        end
    end

    describe "#to_simple_value" do
        it "returns the numerical value" do
            int_t = registry.get '/int32_t'
            int = int_t.new
            int.zero!
            assert_equal 0, int.to_simple_value
        end
    end

    describe "#to_json_value" do
        attr_reader :int_t, :float_t
        before do
            @int_t   = registry.get '/int'
            @float_t = registry.get '/float'
        end
        it "returns an integer as-is" do
            int = Typelib.from_ruby(0, int_t)
            assert_equal 0, int.to_json_value
        end
        it "returns a normal float as-is" do
            float = Typelib.from_ruby(0.0, float_t)
            assert_equal 0.0, float.to_json_value
        end

        describe "convertion to nil" do
            it "converts a NaN into nil" do
                float = Typelib.from_ruby(Float::NAN, float_t)
                assert_nil float.to_json_value(:special_float_values => :nil)
            end
            it "converts a positive infinity into nil" do
                float = Typelib.from_ruby(Float::INFINITY, float_t)
                assert_nil float.to_json_value(:special_float_values => :nil)
            end
            it "converts a negative infinity into nil" do
                float = Typelib.from_ruby(-Float::INFINITY, float_t)
                assert_nil float.to_json_value(:special_float_values => :nil)
            end
        end

        describe "convertion to string" do
            it "converts a NaN into a string" do
                float = Typelib.from_ruby(Float::NAN, float_t)
                assert_equal "NaN", float.to_json_value(:special_float_values => :string)
            end
            it "converts a positive infinity into a string" do
                float = Typelib.from_ruby(Float::INFINITY, float_t)
                assert_equal "Infinity", float.to_json_value(:special_float_values => :string)
            end
            it "converts a negative infinity into a string" do
                float = Typelib.from_ruby(-Float::INFINITY, float_t)
                assert_equal "-Infinity", float.to_json_value(:special_float_values => :string)
            end
        end
    end

    describe "the bool type" do
        attr_reader :bool_t
        before do
            registry = Typelib::CXXRegistry.new
            @bool_t = registry.get '/bool'
        end

        it "is converted to ruby false if zero" do
            v = bool_t.new
            v.zero!
            assert_same false, Typelib.to_ruby(v)
        end
        it "is converted to ruby true if non-zero" do
            v = Typelib.from_ruby(rand(100) + 1, bool_t)
            assert_same true, Typelib.to_ruby(v)
        end
        it "can be set from a Ruby true value" do
            v = Typelib.from_ruby(true, bool_t)
            expected = Typelib.from_ruby(1, bool_t)
            assert_equal expected, v
        end
        it "can be set from a Ruby false value" do
            v = Typelib.from_ruby(false, bool_t)
            expected = Typelib.from_ruby(0, bool_t)
            assert_equal expected, v
        end
    end
end

