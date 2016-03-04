require 'typelib/test'
require_relative './cxx_common_tests'
require_relative './cxx_gccxml_common'

class TC_CXX_CastXML < Minitest::Test
    include Typelib
    include CXXCommonTests

    def setup
        super
        setup_loader 'castxml'
    end

    include CXX_GCCXML_Common
end

