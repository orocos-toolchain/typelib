require 'typelib/test'

class TC_TLB < Minitest::Test

    # The bulk of the C++ tests are made of a C++ file and an expected tlb file.
    # This method generate one test method per such file
    #
    # @param [String] dir the directory containing the tests
    def self.generate_common_tests(dir)
        singleton_class.class_eval do
            define_method(:tlb_test_dir) { dir }
        end

        Dir.glob(File.join(dir, '*.tlb')) do |file|
            basename = File.basename(file, '.tlb')
            prefix   = File.join(dir, basename)
            expected = "#{prefix}.expected"
            next if !File.file?(expected)

            define_method "test_tlb_#{basename}" do
                reg = Typelib::Registry.new
                expected = Typelib::Registry.from_xml(File.read(expected))
                reg.import(file, 'tlb')

                expected.each(with_aliases: true) do |name, expected_type|
                    actual_type = reg.build(name)

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

    tlb_test_dir = File.expand_path('tlb_import_tests', File.dirname(__FILE__))
    generate_common_tests(tlb_test_dir)

    def tbl_test_dir
        CXXCommonTests.tlb_test_dir
    end
end
