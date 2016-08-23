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

