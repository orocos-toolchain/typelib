require 'typelib/test'
require_relative './cxx_test_helpers'

class TC_CXX_Clang < Minitest::Test
    include Typelib

    extend CXXTestHelpers
    cxx_test_dir = File.expand_path('cxx_import_tests', File.dirname(__FILE__))
    generate_common_tests('clang', cxx_test_dir)
end

