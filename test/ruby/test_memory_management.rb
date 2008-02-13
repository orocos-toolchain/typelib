require 'test_config'
require 'typelib'
require 'test/unit'


class TC_MemoryManagement < Test::Unit::TestCase
    def setup
	@finalized = Array.new
	super
    end

    class FailedFinalizationCheck < RuntimeError; end

    def cleanup_stack(depth = 10, *args, &block)
	if depth > 0
	    cleanup_stack(depth - 1, *args, &block)
	else
	    yield if block_given?
	end
	nil
    end

    def did_finalize(finalized_id)
	for info in @finalized.last
	    if info[0] == finalized_id
		info[1] = true
	    end
	end
	nil
    end

    def assert_finalization(&block)
	@finalized << Array.new
	cleanup_stack(&block)
	GC.start
	@finalized.last.each do |id, flag, test_flag, desc|
	    if test_flag
		if flag != test_flag
		    raise FailedFinalizationCheck, "#{desc} has not been finalized"
		end
	    else
		if flag != test_flag
		    raise FailedFinalizationCheck, "#{desc} has been finalized"
		end
	    end
	end

    ensure
	@finalized.pop
    end

    def check_finalization(object, test_finalized, level = 0)
	@finalized[-level - 1] << [object.object_id, false, test_finalized, object.inspect]
	ObjectSpace.define_finalizer(object, method(:did_finalize))
	nil
    end

    def make_registry
        registry = Typelib::Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile  ) }
        registry.import( testfile, "c" )

        registry
    end

    def test_check_tools
	kept_ref = nil

	assert_raises(FailedFinalizationCheck) do
	    assert_finalization do
		kept_ref = Object.new
		check_finalization(kept_ref, true)
	    end
	end

	assert_nothing_raised do
	    assert_finalization do
		kept_ref = Object.new
		check_finalization(kept_ref, false)
	    end
	end

	assert_nothing_raised do
	    assert_finalization do
		check_finalization(Object.new, true)
	    end
	end

	assert_raises(FailedFinalizationCheck) do
	    assert_finalization do
		check_finalization(Object.new, false)
	    end
	end

	assert_raises(FailedFinalizationCheck) do
	    assert_finalization do
		assert_nothing_raised do
		    assert_finalization do
			kept_ref = Object.new
			check_finalization(kept_ref, true, 1)
		    end
		end
	    end
	end
    end

    def test_finalize_value_memory
	registry = make_registry
	type = registry.get("/int")
	assert_finalization do
	    check_finalization(value = type.new, true)
	    check_finalization(value.instance_variable_get(:@ptr), true)
	end
    end

    def test_array_handling
	registry = make_registry
	type = registry.build("/struct B[100]")

	assert_finalization do
	    assert_finalization do
		array = type.new
		first_el = array[0]
		other_el = array[20]
		assert_same(first_el.instance_variable_get(:@ptr), array.instance_variable_get(:@ptr))
		assert_not_same(other_el.instance_variable_get(:@ptr), array.instance_variable_get(:@ptr))

		check_finalization(array, true)
		check_finalization(array.instance_variable_get(:@ptr), true)
		check_finalization(first_el, true)
		check_finalization(other_el, true)
	    end

	    other_el = nil
	    assert_finalization do
		array = type.new
		first_el = array[0]
		other_el = array[20]

		check_finalization(array, false)
		check_finalization(array.instance_variable_get(:@ptr), false)
		check_finalization(array, true, 1)
		check_finalization(array.instance_variable_get(:@ptr), true, 1)
		check_finalization(first_el, true)
		check_finalization(other_el, false)
	    end

	    check_finalization(other_el, true)
	end
    end

    def test_memory_handling
	registry = make_registry
	type   = registry.build("/struct B")
	struct = type.new

	test = type.wrap(struct.instance_variable_get(:@ptr))
	assert_same(struct.instance_variable_get(:@ptr), test.instance_variable_get(:@ptr))

	struct.a
    end

    def test_structure_handling
	registry = make_registry
	type = registry.build("/struct B")

	assert_finalization do
	    other_el = nil

	    assert_finalization do
	         struct = type.new
	         first_el = struct.a

		 assert_same(struct.instance_variable_get(:@ptr), first_el.instance_variable_get(:@ptr))

	         check_finalization(struct, true)
	         check_finalization(struct.instance_variable_get(:@ptr), true)
	         check_finalization(first_el, true)
	    end

	    # assert_finalization do
	    #     struct = type.new
	    #     first_el = struct.a
	    #     other_el = struct.c
	    #     assert_same(first_el.instance_variable_get(:@ptr), struct.instance_variable_get(:@ptr))
	    #     assert_not_same(other_el.instance_variable_get(:@ptr), struct.instance_variable_get(:@ptr))

	    #     check_finalization(struct, false)
	    #     check_finalization(struct, true, 1)
	    #     check_finalization(struct.instance_variable_get(:@ptr), false)
	    #     check_finalization(struct.instance_variable_get(:@ptr), true, 1)
	    #     check_finalization(first_el, false)
	    #     check_finalization(first_el, true, 1)
	    #     check_finalization(other_el, false)
	    # end

	    # check_finalization(other_el, true)
	end
    end

    def test_to_ptr
	registry = make_registry
	type = registry.build("/struct B")

	assert_finalization do
	    assert_finalization do
		value = type.new
		ptr   = value.to_ptr
		assert_same(ptr.deference, value)

		check_finalization(value, true)
		check_finalization(value.instance_variable_get(:@ptr), true)
		check_finalization(ptr, true)
	    end

	    ptr = nil
	    assert_finalization do
		value = type.new
		ptr = value.to_ptr
		assert_same(ptr.deference, value)

		check_finalization(value, false)
		check_finalization(value.instance_variable_get(:@ptr), false)
		check_finalization(value, true, 1)
		check_finalization(value.instance_variable_get(:@ptr), true, 1)
		check_finalization(ptr, false)
	    end
	    check_finalization(ptr, true)
	end
    end

    def test_dup_separate_entities
	registry = make_registry
	type = registry.get("/int")
	original_value = type.new
	assert_finalization do
	    dup = original_value.dup
	    check_finalization(dup, true)
	    check_finalization(dup.instance_variable_get(:@ptr), true)
	    check_finalization(original_value, false)
	    check_finalization(original_value.instance_variable_get(:@ptr), false)
	end
    end

    def test_complete
	registry = make_registry
	type = registry.get("/TestMemoryManagement")

	assert_finalization do
	    deep = nil

	    assert_finalization do
		value = type.new
		b = value.b
		array = b[5]
		deep = array[5]

		check_finalization(value, false)
		check_finalization(b, false)
		check_finalization(array, false)
		check_finalization(deep, false)

		check_finalization(value, true, 1)
		check_finalization(b, true, 1)
		check_finalization(array, true, 1)
	    end
	    check_finalization(deep, true)
	end

	assert_finalization do
	    deep = nil

	    assert_finalization do
		value = type.new
		b = value.b
		array = b[0]
		deep = array[0]

		check_finalization(value, false)
		check_finalization(b, false)
		check_finalization(array, false)
		check_finalization(deep, false)

		check_finalization(value, true, 1)
		check_finalization(b, true, 1)
		check_finalization(array, true, 1)
	    end
	    check_finalization(deep, true)
	end
    end

    def test_memory_to_ptr
	registry = make_registry
	dlptr = nil
	assert_finalization do
	    value = registry.get("/TestMemoryManagement").new
	    dl = value.instance_variable_get("@ptr")
	    assert(dl)
	    dlptr = dl.to_ptr
	    assert(dlptr)

	    check_finalization(value, true)
	    check_finalization(dl, false)
	    check_finalization(dlptr, false)
	end
    end
end

