require 'typelib/test'

class TC_Value < Minitest::Test
    include Typelib
    def teardown
        super
        GC.start
    end

    class CXXRegistry < Typelib::Registry
        def initialize
            super
            Typelib::Registry.add_standard_cxx_types(self)
        end
    end

    # Not in setup() since we want to make sure
    # that the registry is not destroyed by the GC
    def make_registry
        registry = Registry.new
        testfile = File.join(SRCDIR, "test_cimport.1")
        assert_raises(RuntimeError) { registry.import( testfile ) }
        registry.import( testfile, "c" )
        registry
    end

    def test_to_ruby
        int = CXXRegistry.new.get("/int").new
        int.zero!
        assert_equal(0, int.to_ruby)

        str = CXXRegistry.new.build("/char[20]").new
        base_val = if ?a.respond_to?(:ord) then ?a.ord
                   else ?a
                   end
        20.times do |i|
            str[i] = base_val + i
        end
        assert_kind_of(String, str.to_ruby)
    end

    def test_wrapping_a_buffer_should_call_typelib_initialize
        int_t = CXXRegistry.new.build("/int")
        v = Typelib.from_ruby(2, int_t)
        recorder = flexmock
        recorder.should_receive(:initialized).once
        int_t.class_eval do
            define_method(:typelib_initialize) { recorder.initialized }
        end
        int_t.from_buffer(v.to_byte_array)
    end

    def test_it_creates_a_pre_zeroed_out_then_initialized_value_if_the_global_zero_all_values_attribute_is_set_to_true
        registry = Typelib::CXXRegistry.new
        registry.create_enum "/E" do |e|
            e.add "First", 1
            e.add "Second", 2
        end
        type = registry.create_compound "/ZeroTest" do |c|
            c.add "e", "/E"
            c.add "uninit", "/int"
            c.add "init", "/int"
        end

        type.define_method(:initialize) { self.init = 42 }

        Typelib.zero_all_values = true
        value = type.new
        assert_equal :First, value.e
        assert_equal 0, value.uninit
        assert_equal 42, value.init
    ensure
        Typelib.zero_all_values = false
    end

    def test_zero_creates_a_pre_zeroed_out_then_initialized_value
        registry = Typelib::CXXRegistry.new
        registry.create_enum "/E" do |e|
            e.add "First", 1
            e.add "Second", 2
        end
        type = registry.create_compound "/ZeroTest" do |c|
            c.add "e", "/E"
            c.add "uninit", "/int"
            c.add "init", "/int"
        end

        type.define_method(:initialize) { self.init = 42 }

        value = type.zero
        assert_equal :First, value.e
        assert_equal 0, value.uninit
        assert_equal 42, value.init
    end

    def test_wrapping_a_memory_zone_should_call_typelib_initialize
        int_t = CXXRegistry.new.build("/int")
        v = Typelib.from_ruby(2, int_t)
        flexmock(int_t).new_instances.should_receive(:typelib_initialize).once
        int_t.wrap(v.to_memory_ptr)
    end

    def test_creating_a_new_value_should_call_typelib_initialize
        int_t = CXXRegistry.new.build("/int")
        recorder = flexmock
        recorder.should_receive(:initialized).once
        int_t.class_eval do
            define_method(:typelib_initialize) { recorder.initialized }
        end
        int_t.new
    end

    def test_wrapping_a_buffer_should_not_call_initialize
        int_t = CXXRegistry.new.build("/int")
        v = Typelib.from_ruby(2, int_t)
        flexmock(int_t).new_instances.should_receive(:initialize).never
        int_t.wrap(v.to_byte_array)
    end

    def test_wrapping_a_memory_zone_should_not_call_initialize
        int_t = CXXRegistry.new.build("/int")
        v = Typelib.from_ruby(2, int_t)
        flexmock(int_t).new_instances.should_receive(:initialize).never
        int_t.wrap(v.to_memory_ptr)
    end

    def test_creating_a_new_value_should_call_initialize
        int_t = CXXRegistry.new.build("/int")
        flexmock(int_t).new_instances(:value_new).should_receive(:initialize).once
        int_t.new
    end

    def test_wrap_argument_check
        type = CXXRegistry.new.build("/int")

        assert_raises(ArgumentError) { type.wrap(nil) }
        assert_raises(ArgumentError) { type.wrap(10) }
    end

    def test_value_equality
        type = CXXRegistry.new.build("/int")
        v1 = type.new.zero!
        v2 = type.new.zero!
        assert_equal(0, v1)
        assert_equal(v1, v2)
        assert(! v1.eql?(v2))

        # This one is tricky: we want to have == return true if the fields of a
        # compounds are equal, regardless of padding bytes. So we prepare a
        # pattern which will be used as "blank" memory and then fill the fields
        registry = make_registry
        a_type    = registry.get("/A")
        a_pattern = (1..a_type.size).to_a.pack("c*")
        a1 = a_type.wrap a_pattern
        a2 = a_type.wrap a_pattern.reverse
        a1.a = a2.a = 10
        a1.b = a2.b = 20
        a1.c = a2.c = 30
        a1.d = a2.d = 40
        assert_equal(a1, a2)
        assert(! a1.eql?(a2))

        a2.d = 50
        refute_equal(a1, a2)

        assert_raises(ArgumentError) { a1 == v1 }
    end

    def test_value_cast
        r0 = make_registry
        r1 = make_registry
        t0 = r0.get 'StdCollections'
        t1 = r1.get 'StdCollections'

        v0 = t0.new
        assert_same v0, v0.cast(t0)
        v1 = v0.cast(t1)

        refute_same v0, v1
        assert(t0 == t1)

        wrong_type = r1.get 'A'
        assert_raises(ArgumentError) { v0.cast(wrong_type) }
    end

    def test_byte_array
        as_string = [5].pack('S')
        long_t = CXXRegistry.new.build("/short")

        assert_raises(ArgumentError) { long_t.wrap "" }
        rb_value = long_t.wrap as_string
        as_string = rb_value.to_byte_array
        assert_equal(2, as_string.size)
        assert_equal(5, as_string.unpack('S').first)

        a = make_registry.get('ADef')
        # The following line will lead to valgrind complaining. This is
        # harmless: we access the attribute values before we assign them.
        a = a.new :a => 10, :b => 20, :c => 30, :d => 40
        assert_equal([10, 20, 30, 40], a.to_byte_array.unpack('Qicxs'))
    end

    def test_string_handling
        buffer_t = CXXRegistry.new.build("/char[256]")
        buffer = buffer_t.new
        assert( buffer.string_handler? )
        assert( buffer.respond_to?(:to_str))

        # Check that .from_str.to_str is an identity
        typelib_value = Typelib.from_ruby("first test", buffer_t)
        assert_kind_of buffer_t, typelib_value
        assert_equal("first test", typelib_value.to_ruby)
        assert_raises(ArgumentError) { Typelib.from_ruby("a"*512, buffer_t) }
    end

    def test_pretty_printing
        b = make_registry.get("/B").new
        b.zero!
        # Should not raise
        PP.new(b, StringIO.new)
    end

    def test_to_csv
        klass = make_registry.get("/DisplayTest")
        assert_equal("t.fields[0] t.fields[1] t.fields[2] t.fields[3] t.f t.d t.a.a t.a.b t.a.c t.a.d t.mode", klass.to_csv('t'));
        assert_equal(".fields[0] .fields[1] .fields[2] .fields[3] .f .d .a.a .a.b .a.c .a.d .mode", klass.to_csv);
        assert_equal(".fields[0],.fields[1],.fields[2],.fields[3],.f,.d,.a.a,.a.b,.a.c,.a.d,.mode", klass.to_csv('', ','));

        value = klass.new
        value.zero!
        value.fields[0] = 0;
        value.fields[1] = 1;
        value.fields[2] = 2;
        value.fields[3] = 3;
        value.f = 1.1;
        value.d = 2.2;
        value.a.a = 10;
        value.a.b = 20;
        value.a.c = 'b';
        value.a.d = 42;

        space_sep = value.to_csv.split(" ")
        assert_in_delta(1.1, Float(space_sep[4]), 0.00001)
        assert_in_delta(2.2, Float(space_sep[5]), 0.00001)
        space_sep[4] = "1.1"
        space_sep[5] = "2.2"
        assert_equal("0 1 2 3 1.1 2.2 10 20 98 42 OUTPUT", space_sep.join(" "))

        comma_sep = value.to_csv(",").split(",")
        assert_in_delta(1.1, Float(space_sep[4]), 0.00001)
        assert_in_delta(2.2, Float(space_sep[5]), 0.00001)
        comma_sep[4] = "1.1"
        comma_sep[5] = "2.2"
        assert_equal("0,1,2,3,1.1,2.2,10,20,98,42,OUTPUT", comma_sep.join(","))
    end


    def test_is_a
        assert !Typelib::CompoundType.is_a?("/A")

        registry = make_registry
        a = registry.get("/A").new
        assert( a.is_a?("/A") )
        assert( a.is_a?(/A$/) )

        assert( a.is_a?(registry.get("/A")) )
        assert( a.is_a?(registry.get("/int64_t")) )
    end

    def test_dup
        registry = make_registry
        a = registry.get("/A").new
        a.a = 20
        b = a.dup

        assert_kind_of(a.class, b)

        assert_equal(20, b.a)
        b.a = 10
        assert_equal(20, a.a)
        assert_equal(10, b.a)
    end

    def test_clone
        registry = make_registry
        a = registry.get("/A").new
        a.a = 20
        b = a.clone

        assert_kind_of(a.class, b)

        assert_equal(20, b.a)
        b.a = 10
        assert_equal(20, a.a)
        assert_equal(10, b.a)
    end


    def test_to_s
        int_value = CXXRegistry.new.get("/int").new
        int_value.zero!
        assert_equal("0", int_value.to_s)
    end

    def test_copy
        r0 = make_registry
        r1 = make_registry

        type0 = r0.get "/B"
        type1 = r1.get "/B"

        v0 = type0.new
        v1 = type1.new
        v0.zero!
        v1.zero!
        v0.a.a = 20
        v0.a.b = 60
        v0.a.c = 4
        v0.a.d = 30
        v0.x = 230
        assert !v1.memory_eql?(v0)
        Typelib.copy(v1, v0)
        assert(v1 == v0, "copy failed: memory is not equal")
    end

    def test_nan_handling
        reg = Typelib::CXXRegistry.new

        double_t = reg.get('/double')
        nan_d = [Float::NAN].pack("d")
        assert double_t.wrap(nan_d).to_ruby.nan?

        float_t = reg.get('/float')
        nan_f = [Float::NAN].pack("f")
        assert float_t.wrap(nan_f).to_ruby.nan?
    end

    def test_vector_of_vector
        registry = make_registry
        containers_t = registry.get('Collections')
        v_v_struct_t = containers_t[:v_v_struct]
        v_struct_t   = v_v_struct_t.deference
        struct_t     = v_struct_t.deference

        v_v_struct = v_v_struct_t.new
        v_v_struct << [struct_t.new(a: 10)]
        assert_equal 10, v_v_struct.raw_get(0).raw_get(0).a
    end

    def test_arrays
        registry = make_registry
        array_t = registry.get('Arrays')
        ns1_test_t = registry.get('/NS1/Test')
        arrays = array_t.new
        arrays.zero!
        array_a_struct = arrays.raw_a_struct
        array_a_struct[0] = ns1_test_t.new(a: 10)
        assert_equal [10, 0, 0, 0, 0, 0, 0, 0, 0, 0], array_a_struct.map(&:a)
    end

    def test_convertion_to_from_ruby
        Typelib.convert_to_ruby '/NS1/Test', Integer do |value|
            value.a
        end
        Typelib.convert_from_ruby Fixnum, '/NS1/Test' do |value, expected_type|
            result = expected_type.new
            result.a = value
            result
        end

        registry = make_registry
        ns1_test_t = registry.get('/NS1/Test')
        assert_equal Integer, ns1_test_t.convertion_to_ruby[0]
        vector_ns1_test_t = registry.get('/std/vector</NS1/Test>')
        assert_equal Array, vector_ns1_test_t.convertion_to_ruby[0]
        vector_vector_ns1_test_t = registry.get('/std/vector</std/vector</NS1/Test>>')
        assert_equal Array, vector_vector_ns1_test_t.convertion_to_ruby[0]

        # Test that containers get converted to array because of the custom
        # convertion on /NS1/Test
        containers_t = registry.get('Collections')
        containers = containers_t.new
        v_v_struct = containers.v_v_struct
        assert_kind_of Array, v_v_struct
        assert_same v_v_struct, containers.v_v_struct
        v_v_struct << [10]
        assert_equal [], containers.raw_v_v_struct.to_ruby
        containers.apply_changes_from_converted_types
        assert_equal [[10]], containers.raw_v_v_struct.to_ruby

        # Test that C++ Arrays get converted to Ruby Array because of the custom
        # convertion on /NS1/Test
        array_t = registry.get('Arrays')
        arrays = array_t.new
        arrays.zero!
        a_struct = arrays.a_struct
        assert_kind_of Array, a_struct
        assert_same a_struct, arrays.a_struct
        a_struct[0] = 10
        assert_equal [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], arrays.raw_a_struct.to_ruby
        arrays.apply_changes_from_converted_types
        assert_equal [10, 0, 0, 0, 0, 0, 0, 0, 0, 0], arrays.raw_a_struct.to_ruby
    ensure
        Typelib.convertions_from_ruby.from_typename.delete('/NS1/Test')
        Typelib.convertions_to_ruby.from_typename.delete('/NS1/Test')
    end
end

