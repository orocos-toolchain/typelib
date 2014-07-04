require 'typelib'
require 'minitest/spec'

describe Typelib::EnumType do
    attr_reader :registry
    before do
        @registry = Typelib::CXXRegistry.new
    end

    describe "the type model" do
        describe "#to_h" do
            attr_reader :enum_t
            before do
                @enum_t = registry.create_enum '/E' do |builder|
                    builder.add 'S0', 0
                    builder.add 'S10', 10
                end
            end

            it "should be able to describe the type" do
                expected = Hash['class' => 'Typelib::EnumType',
                                'name' => enum_t.name,
                                'values' => [
                                    Hash['name' => 'S0', 'value' => 0],
                                    Hash['name' => 'S10', 'value' => 10]
                                ]]
                assert_equal expected, enum_t.to_h
            end
        end
    end
end


