require 'typelib/test'

describe Typelib::ContainerType::StdVector do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "#raw_memcpy" do
        attr_reader :value_t
        before do
            @value_t = registry.create_container '/std/vector', '/int32_t'
        end

        it "should successfully copy from a raw address" do
            v0 = value_t.new
            v0 << 0 << 1
            v1 = value_t.new
            v1.raw_memcpy(v0.raw_get(0).to_memory_ptr.zone_address, 8)
            assert_equal 2, v1.size
            assert_equal 0, v1[0]
            assert_equal 1, v1[1]
        end
        it "should validate its size argument to be a fixed number of elements" do
            v = value_t.new
            assert_raises(ArgumentError) do
                v.raw_memcpy(0, 7)
            end
        end
    end
end

