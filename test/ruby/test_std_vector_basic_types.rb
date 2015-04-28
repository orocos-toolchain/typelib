require 'typelib/test'

module Typelib
    %w{int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t double float}.each do |basic_type|
        describe "std/vector of #{basic_type}" do
            attr_reader :value, :vector_t, :v

            before do
                registry = Typelib::CXXRegistry.new
                type = registry.get("/#{basic_type}")
                @vector_t = registry.create_container '/std/vector', type
                @v = vector_t.new
                @value =
                    if basic_type =~ /int/
                        10
                    else
                        10.0
                    end
            end

            describe "#<<" do
                it "adds an element from Ruby values" do
                    v << value
                    assert_equal [value], v.to_a
                end
            end

            describe "#set" do
                it "sets from Ruby values" do
                    v << 0
                    v.set(0, value)
                    assert_equal [value], v.to_a
                end
            end

            describe "#[]=" do
                it "sets from Ruby values" do
                    v << 0
                    v[0] = value
                    assert_equal [value], v.to_a
                end
            end

            describe "#get" do
                it "returns the converted value evne if there is already a cached Typelib value for it" do
                    v << value
                    v.raw_get(0) # cache a typelib value for this element
                    # It is important to use assert_same here, as
                    # Typelib::Numeric#== would return true if checked against a
                    # Ruby numeric
                    assert_same value, v.get(0)
                end

                it "returns the converted value" do
                    v << 0
                    v[0] = value
                    # It is important to use assert_same here, as
                    # Typelib::Numeric#== would return true if checked against a
                    # Ruby numeric
                    assert_same value, v.get(0)
                end
            end

            describe "#[]" do
                it "returns the converted value" do
                    v << 0
                    v[0] = value
                    # It is important to use assert_same here, as
                    # Typelib::Numeric#== would return true if checked against a
                    # Ruby numeric
                    assert_same value, v[0]
                end
            end

            describe "#raw_get" do
                it "returns the typelib value" do
                    v << 0
                    v[0] = value
                    typelib_v = v.raw_get(0)
                    assert_kind_of Typelib::NumericType, typelib_v
                    assert_equal value, typelib_v
                end
            end

            describe "#to_a" do
                it "converts to an array, including all values to the equivalent Ruby numeric" do
                    v << 0
                    v[0] = value
                    a = v.to_a
                    assert_kind_of Array, a
                    # It is important to use assert_same here, as
                    # Typelib::Numeric#== would return true if checked against a
                    # Ruby numeric
                    assert_same value, a[0]
                end
            end
        end
    end
end

