require_relative 'helpers'

Helpers.ips do |x|
    registry = Typelib::Registry.new
    enum_t = registry.create_enum '/enum' do |e|
        e.add :FIRST
        e.add :SECOND
    end
    numeric_t = registry.create_numeric '/numeric', 4, :sint

    compound_numeric_t = registry.dup.create_compound('/C') { |c| c.add 'f', '/numeric' }
    numeric = numeric_t.new
    x.report 'compound.new' do
        compound_numeric_t.new
    end

    x.report 'compound.new(f: ruby_numeric)' do
        compound_numeric_t.new(f: 32)
    end

    x.report 'compound.new(f: typelib_numeric)' do
        compound_numeric_t.new(f: numeric)
    end

    compound_numeric1 = compound_numeric_t.new
    x.report 'compound.numeric = ruby_numeric' do
        compound_numeric1.f = 32
    end

    compound_numeric2 = compound_numeric_t.new
    x.report 'compound.numeric = typelib_numeric' do
        compound_numeric2.f = numeric
    end

    compound_enum_t = registry.dup.create_compound('/C') { |c| c.add 'f', '/enum' }
    enum = enum_t.new
    x.report 'compound.new(f: ruby_enum)' do
        compound_enum_t.new(f: :FIRST)
    end

    x.report 'compound.new(f: typelib_enum)' do
        compound_enum_t.new(f: enum)
    end

    compound_enum1 = compound_enum_t.new
    x.report 'compound.enum = ruby_enum' do
        compound_enum1.f = :FIRST
    end

    compound_enum2 = compound_enum_t.new
    x.report 'compound.enum = typelib_enum' do
        compound_enum2.f = enum
    end

    cc_registry = registry.dup
    compound_t = cc_registry.create_compound('/C') { |c| c.add 'f', '/enum' }
    compound_compound_t = cc_registry.create_compound('/CC') { |c| c.add 'f', '/C' }
    compound = compound_t.new
    x.report 'compound.new(f: compound)' do
        compound_compound_t.new(f: compound)
    end

    compound_compound1 = compound_compound_t.new
    x.report 'compound.compound = compound' do
        compound_compound1.f = compound
    end
end
