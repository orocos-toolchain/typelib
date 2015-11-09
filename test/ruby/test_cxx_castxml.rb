require 'typelib/test'
require_relative './cxx_common_tests'

class TC_CXX_CastXML < Minitest::Test
    include Typelib
    include CXXCommonTests

    def setup
        super
        setup_loader 'castxml'
    end
end

