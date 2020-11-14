#!/usr/bin/env ruby
require 'typelib'

Process.setrlimit(:AS, 300*1024**2)

registry = Typelib::CXXRegistry.new

double_vector_t = registry.create_container '/std/vector', '/double'
particle_t = registry.create_compound '/C' do |c|
  c.add 'f', double_vector_t
end
particle_vector_t = registry.create_container '/std/vector', particle_t

internal = particle_t.new(f: [0]*100_000)
10.times do |i|
  vector = particle_vector_t.new
  10.times do
      vector.push internal
  end
end
