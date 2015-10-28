module CXXTestHelpers
    def generate_common_tests(importer, dir, importer_options = Hash.new)
        loader = Typelib::CXX::CXX_LOADERS[importer]

        Dir.glob(File.join(dir, '*.hh')) do |file|
            basename = File.basename(file, '.hh')
            prefix   = File.join(dir, basename)
            opaques  = "#{prefix}.opaques"

            define_method "test_cxx_common_#{basename}" do
                reg = Typelib::Registry.new
                expected = Typelib::Registry.from_xml(File.read("#{prefix}.tlb"))
                if File.file?(opaques)
                    reg.import(opaques, 'tlb')
                end
                loader.load(reg, file, 'c', importer_options)

                expected.each(with_aliases: true) do |name, expected_type|
                    actual_type = reg.get(name)

                    if expected_type != actual_type
                        error_message = "in #{file}, failed expected and actual definitions type for #{name} differ\n"
                        pp = PP.new(error_message)
                        name.pretty_print(pp)
                        pp.breakable
                        pp.text "Expected: "
                        pp.breakable
                        pp.text expected_type.to_xml
                        pp.breakable
                        pp.text "Actual: "
                        pp.breakable
                        pp.text actual_type.to_xml
                        flunk(error_message)
                    end
                end
            end
        end
    end
end
