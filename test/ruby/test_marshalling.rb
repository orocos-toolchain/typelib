require 'test_config'
require 'typelib'
require 'test/unit'
require '.libs/test_rb_value'
require 'pp'

require 'drb'
require 'stringio'

class TC_Marshalling  < Test::Unit::TestCase
    include Typelib

    def teardown
	DRb.stop_service
    end

    def test_marshal_type
        registry = Registry.new
        registry.import( File.join(SRCDIR, "test_cimport.1"), "c" )

	DRb.start_service
        t = registry.get("/struct A")
	m_t = nil
        assert_nothing_raised { m_t = Marshal.dump(t) }
        um_t = Marshal.load(StringIO.new(m_t))
    end

    def test_marshal_value
        registry = Registry.new
        registry.import( File.join(SRCDIR, "test_cimport.1"), "c" )

	DRb.start_service
        t = registry.get("/struct A")
        v = t.new :a => 10, :b => 20, :c => 30, :d => 40
        assert_nothing_raised { m_v = Marshal.dump(v) }
        um_v = Marshal.load(StringIO.new(m_v))

        assert_equal(10, um_v.a)
        assert_equal(20, um_v.b)
        assert_equal(30, um_v.c)
        assert_equal(40, um_v.d)
    end

    def test_drb
	pid = fork do
	    registry = Registry.new
	    registry.import( File.join(SRCDIR, "test_cimport.1"), "c" )

	    front = Class.new do
		define_method(:get) do
		    t = registry.get("/struct A")
		    t.new :a => 10, :b => 20, :c => 30, :d => 40
		end
	    end.new

	    DRb.start_service 'druby://localhost:4765', front
	    DRb.thread.join
	end

	sleep 1
	DRb.start_service 'druby://localhost:4766'
	remote = DRbObject.new_with_uri('druby://localhost:4765')
	v = remote.get
	assert_equal(10, v.a)
	assert_equal(20, v.b)
	assert_equal(30, v.c)
	assert_equal(40, v.d)

    ensure
	Process.kill 'KILL', pid
    end
end

