require 'rubygems'
require 'bundler'
begin
  Bundler.setup(:default, :development)
rescue Bundler::BundlerError => e
  $stderr.puts e.message
  $stderr.puts "Run `bundle install` to install missing gems"
  exit e.status_code
end
require 'rake'

require 'jeweler'
jeweler_tasks = Jeweler::Tasks.new do |gem|
  # gem is a Gem::Specification... see http://docs.rubygems.org/read/chapter/20 for more options
  gem.name = "requencer"
  gem.homepage = "http://github.com/thedjinn/requencer"
  gem.license = "MIT"
  gem.summary = %Q{Ruby audio sequencer/mixer}
  gem.description = gem.summary
  gem.email = "emil@koffietijd.net"
  gem.authors = ["Emil Loer"]
  gem.extensions = FileList["ext/**/extconf.rb"]
#  gem.files.include('lib/requencer/requencer.*')
  # Include your dependencies below. Runtime dependencies are required when using your gem,
  # and development dependencies are only needed for development (ie running rake tasks, tests, etc)
  #  gem.add_runtime_dependency 'jabber4r', '> 0.1'
  #  gem.add_development_dependency 'rspec', '> 1.2.3'
end
Jeweler::RubygemsDotOrgTasks.new

$gemspec = jeweler_tasks.gemspec
$gemspec.version = jeweler_tasks.jeweler.version

require 'rspec/core'
require 'rspec/core/rake_task'
RSpec::Core::RakeTask.new(:spec) do |spec|
  spec.pattern = FileList['spec/**/*_spec.rb']
end

RSpec::Core::RakeTask.new(:rcov) do |spec|
  spec.pattern = 'spec/**/*_spec.rb'
  spec.rcov = true
end

task :default => :spec

require 'rake/rdoctask'
Rake::RDocTask.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "Requencer #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('lib/**/*.rb')
end


require 'rake/extensiontask'
Rake::ExtensionTask.new('requencer', $gemspec) do |ext|
  ext.lib_dir = File.join("lib", "requencer")
end

CLEAN.include 'lib/**/*.so'

#---- old
#Rake::ExtensionTask.new("requencer", spec) do |ext|
#    ext.lib_dir = File.join("lib", "requencer")
#  end
#
#  desc "Compile Doxygen documentation"
#  task :doxygen do
#    sh "doxygen"
#  end
#end
