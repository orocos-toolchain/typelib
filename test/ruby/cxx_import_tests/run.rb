require 'typelib'

reg = Typelib::Registry.new
Dir.glob('*.hh') do |file|
    dir = Dir.pwd
    basename = File.basename(file, '.hh')
    expected = Typelib::Registry.from_xml(File.read("#{basename}.tlb"))
    opaques  = "#{basename}.opaques"
    if File.file?(opaques)
        reg.import(opaques, 'tlb')
    end

    reg.import(file)

    expected.each(with_aliases: true) do |name, type|
        if reg.get(name) != type
            STDERR.puts "FAILED #{file}"
            pp = PP.new(STDERR)
            name.pretty_print(pp)
            pp.breakable
            pp.text "Expected: "
            type.pretty_print(pp, true)
            pp.breakable
            pp.text "Actual: "
            reg.get(name).pretty_print(pp, true)
            binding.pry
            "not matching: #{name}"
        end
    end
end

