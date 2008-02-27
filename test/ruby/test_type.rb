require 'set'
require 'test_config'
require 'typelib'
require 'test/unit'
require BUILDDIR + '/ruby/libtest_ruby'
require 'pp'

class TC_Type < Test::Unit::TestCase
    include Typelib
    def teardown
	GC.start
    end

    # Not in setup() since we want to make sure
    # that the registry is not destroyed by the GC
    def make_registry
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        registry
    end

    def test_aliasing
	registry = Registry.new
	registry.alias "/my_own_and_only_int", "/int"
	assert_equal(registry.get("my_own_and_only_int"), registry.get("int"))
	assert_equal("/int", registry.get("/my_own_and_only_int").name)
    end

    def test_registry
        assert_raises(RuntimeError) { Registry.guess_type("bla.1") }
        assert_equal("c", Registry.guess_type("blo.c"))
        assert_equal("c", Registry.guess_type("blo.h"))
        assert_equal("tlb", Registry.guess_type("blo.tlb"))

        assert_raises(RuntimeError) { Registry.new.import("bla.c") }

        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.h")
        assert_raises(RuntimeError) { registry.import(testfile) }
        assert_nothing_raised {
            registry.import(testfile, nil, :include => [ File.join(SRCDIR, '..') ], :define => [ 'GOOD' ])
        }

        registry = Registry.new
        assert_nothing_raised {
            registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
        }
        assert_nothing_raised {
            registry.import(testfile, nil, :merge => true, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ])
        }
	assert_raises(RuntimeError) { registry.import(testfile, nil, :rawflag => [ "-I#{File.join(SRCDIR, '..')}", "-DGOOD" ]) }
    end

    def test_registry_iteration
	reg = make_registry
	values = nil
	assert_nothing_raised { values = reg.enum_for(:each_type).to_a }

	assert_not_equal(0, values.size)
	assert(values.include?(reg.get("/int")))
	assert(values.include?(reg.get("/struct EContainer")))

	assert_nothing_raised { values = reg.enum_for(:each_type, true).to_a }
	assert_not_equal(0, values.size)
	assert(values.include?(["/struct EContainer", reg.get("/struct EContainer")]))
	assert(values.include?(["/EContainer", reg.get("/EContainer")]))
    end

    def test_type_inequality
        # Check that == returns false when the two objects aren't of the same class
        # (for instance type == nil shall return false)
	type = nil
        type = Registry.new.get("/int")
        assert_equal("/int", type.name)
        assert_nothing_raised { type == nil }
        assert(type != nil)
    end

    def test_type_names
	registry = make_registry
	bla = registry.get "/NS1/Bla/Test"
	assert_equal("/NS1/Bla/Test", bla.name)
	assert_equal("Test", bla.basename)
	assert_equal("/NS1/Bla/", bla.namespace)
	assert_equal("::NS1::Bla::", bla.namespace('::'))
    end

    def test_pointer
        type = Registry.new.build("/int*")
        assert_not_equal(type, type.deference)
        assert_not_equal(type, type.to_ptr)
        assert_not_equal(type.to_ptr, type.deference)
        assert_equal(type, type.deference.to_ptr)
        assert_equal(type, type.to_ptr.deference)
    end
end


