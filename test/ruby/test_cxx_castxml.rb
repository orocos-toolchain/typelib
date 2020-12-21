require 'typelib/test'
require_relative './cxx_common_tests'
require_relative './cxx_gccxml_common'
require 'typelib/gccxml'

module Typelib
    describe CastXMLLoader do
        include Typelib
        include CXXCommonTests

        before do
            setup_loader 'castxml'
        end

        describe "file resolution" do
            it "normalizes the path of a file whose path was given relative" do
                registry = Registry.new
                relative_path = File.join(__dir__, "cxx_import_tests",
                                          "unsupported", "..", "classes.hh")
                normalized_path = File.join(__dir__, "cxx_import_tests", "classes.hh")
                GCCXMLLoader.load(registry, relative_path, "c", castxml: true)

                type = registry.get("/classes/BaseClass")
                assert_equal ["#{normalized_path}:15"],
                             type.metadata.get("source_file_line")
            end

            it "normalizes the path of includes that are resolved relatively" do
                registry = Registry.new
                path = File.join(__dir__, "cxx_import_tests", "relative_includes.hh")
                include_path = File.join(__dir__, "cxx_import_tests", "unsupported", "..")
                normalized_path = File.join(__dir__, "cxx_import_tests", "classes.hh")
                GCCXMLLoader.load(registry, path, "c",
                                  include_paths: [include_path],
                                  castxml: true)

                type = registry.get("/classes/BaseClass")
                assert_equal ["#{normalized_path}:15"],
                             type.metadata.get("source_file_line")
            end

            it "does the full path normalization during parsing" do
                registry = Registry.new
                main_relative_path =
                    File.join(__dir__, "cxx_import_tests",
                              "unsupported", "..", "relative_includes.hh")
                main_normalized_path =
                    File.join(__dir__, "cxx_import_tests", "relative_includes.hh")
                normalized_path = File.join(__dir__, "cxx_import_tests", "classes.hh")

                include_path = File.join(__dir__, "cxx_import_tests", "unsupported", "..")
                xml = GCCXMLLoader.castxml(
                    main_relative_path, include_paths: [include_path]
                ).first
                info = GCCXMLInfo.new([main_normalized_path, normalized_path])
                info.parse(xml)
                id = info.required_files[normalized_path]
                assert_equal normalized_path, info.id_to_node[id].attributes["name"]
                id = info.required_files[main_normalized_path]
                assert_equal main_normalized_path, info.id_to_node[id].attributes["name"]
            end
        end

        describe "#find_node_by_name" do
            attr_reader :parser, :info
            before do
                @parser = loader.new
                @info = GCCXMLInfo.new(Array.new)
                parser.info = info
            end

            it "resolves from cache" do
                expected = flexmock
                info.name_to_nodes['test'] = [expected]
                assert_equal expected, parser.find_node_by_name('test')
            end
            it "filters on node_type when resolving from cache" do
                expected = flexmock
                expected.should_receive(name: 'Test')
                expected.name
                info.name_to_nodes['test'] = [expected]
                assert_equal expected, parser.find_node_by_name('test', node_type: 'Test')
                refute parser.find_node_by_name('test', node_type: 'Another')
            end
        end
    end
end

