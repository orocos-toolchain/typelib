require 'benchmark/ips'
require 'typelib'
require 'csv'

# Common parts of the Typelib benchmarks
module Helpers
    # A benchmark-ips suite object that saves results in a CSV file
    class CSVSuite
        def initialize(commit_id, output_file)
            @output_file = output_file
            if File.file?(output_file)
                @data = CSV.read(output_file)
                @header = @data.shift
            else
                @data = []
                @header = ['commit_id']
            end
            @report = [commit_id]
        end

        def warming(*); end

        def warmup_stats(*); end

        def running(*); end

        def add_report(entry, *)
            unless (column = @header.index(entry.label))
                column = @header.size
                @header << entry.label << "#{entry.label} (error)"
            end
            @report[column] = entry.stats.central_tendency
            @report[column + 1] = entry.stats.error
        end

        def save
            CSV.open(@output_file, 'w') do |csv|
                csv << @header
                expected_size = @header.size
                @data.each do |l|
                    if l.size < expected_size
                        l[expected_size - 1] = nil
                    end
                    csv << l
                end
                csv << @report
            end
        end
    end

    def self.ips
        csv_file_path = nil
        csv_label     = nil
        parser = OptionParser.new do |opt|
            opt.on '--csv=PATH', String,
                'path of a CSV file the benchmark results should be saved into' do |path|
                csv_file_path = path
            end
            opt.on '--csv-label=LABEL', String,
                'the label of the benchmark entry' do |label|
                csv_label = label
            end
        end
        parser.parse(ARGV)

        ips_args = Hash.new
        if csv_file_path
            raise "no label given with --csv-label" unless csv_label
            puts "Saving benchmark results in #{csv_file_path} as #{csv_label}"
            suite = CSVSuite.new(csv_label, csv_file_path)
            ips_args = { suite: suite }
        end
        Benchmark.ips do |x|
            x.config stats: :bootstrap, confidence: 95, **ips_args
            yield(x)
        end
        suite.save if suite
    end
end
