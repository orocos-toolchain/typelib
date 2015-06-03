require 'typelib/test'

describe Typelib do
    describe ".compare" do
        it "applies changes from converted types first" do
            registry = Typelib::CXXRegistry.new
            field_t    = registry.create_compound '/Field' do |c|
                c.add 'value', 'double'
            end
            field_t.convert_to_ruby Time do |time|
                Time.at(time.value)
            end
            field_t.convert_from_ruby Time do |time, _|
                field_t.new(value: (time - Time.at(0)))
            end
            compound_t = registry.create_compound '/Test' do |c|
                c.add 'field', '/Field'
            end

            c0 = compound_t.new
            c0.raw_field.value = 10
            c1 = compound_t.new
            c1.raw_field.value = 1
            c1.field = Time.at(10)
            assert_equal c0, c1
        end
    end
end
