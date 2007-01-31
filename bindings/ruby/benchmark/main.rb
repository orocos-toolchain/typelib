FILE_DIR=File.expand_path(File.dirname(__FILE__))
TEST_DIR=File.join(FILE_DIR, '..', '..', '..', 'test')
$LOAD_PATH.unshift File.join(FILE_DIR, '..', 'ext', '.libs')
$LOAD_PATH.unshift File.join(FILE_DIR, '..', 'lib')
require 'benchmark'
require 'typelib'

include Typelib

COUNT=10_000
Benchmark.bmbm(7) do |x|
    registry = Registry.new
    registry.import( File.join(TEST_DIR, 'test_cimport.1'), 'c' )

    int_t = registry.get("int")
    int = int_t.new
    b_t = registry.get("struct B")
    b = b_t.new

    x.report("int (#{COUNT})") { COUNT.times { |i| int.to_ruby } }

    a = b.a
    ra_t = Struct.new :a
    ra = ra_t.new
    x.report("Ruby field") { COUNT.times { |i| ra.a } }
    x.report("int field") { COUNT.times { |i| a.a } }
    x.report("struct field") { COUNT.times { |i| b.a } }
    x.report("array field") { COUNT.times { |i| b.c } }

    rarray = Array.new(b.c.size)
    x.report("Ruby iter") { COUNT.times { |i| rarray.each { |v| } } }
    array = b.c
    x.report("array iter (#{b.c.size})") { COUNT.times { |i| array.each { |v| } } }
end

