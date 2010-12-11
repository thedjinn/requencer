# -*- ruby -*-

require 'rubygems'
require 'hoe'
require 'rake/extensiontask'

HOE = Hoe.spec 'requencer' do
  developer('Emil Loer', 'emil@koffietijd.net')

  self.readme_file = "README.rdoc"
  self.history_file = "CHANGELOG.rdoc"
  self.extra_rdoc_files = FileList["*.rdoc"]
  self.extra_dev_deps << ["rake-compiler", ">= 0"]
  self.spec_extras = { :extensions => ["ext/requencer/extconf.rb"] }

  Rake::ExtensionTask.new("requencer", spec) do |ext|
    ext.lib_dir = File.join("lib", "requencer")
  end

  task :test do
    p HOE
    cmdline = "#{HOE.make_test_cmd}"
    puts cmdline
  end

  desc "Compile Doxygen documentation"
  task :doxygen do
    sh "doxygen"
  end
end

#namespace :spec do
#  VALGRIND_BASIC_OPTS = "--num-callers=50 --error-limit=no --partial-loads-ok=yes --undef-value-errors=no --workaround-gcc296-bugs=yes"
#
#  desc "run specs under valgrind"
#  task :valgrind => :compile do
#    cmdline = "valgrind #{VALGRIND_BASIC_OPTS} ruby #{HOE.make_test_cmd}"
#    puts cmdline
#    system cmdline
#  end
#end

Rake::Task[:spec].prerequisites << :compile

# vim: syntax=ruby
