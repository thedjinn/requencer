require File.join(File.dirname(__FILE__), 'spec_helper')

describe "Requencer" do
  subject { Requencer }
  
  it { should respond_to :render }

  describe "#render" do
    context "given invalid input" do
      it "should raise an exception when second argument is not an array" do
        expect { subject.render("String","Not an array of hashes") }.to raise_error
      end
      
      it "should raise an exception when first argument is not a string" do
        expect { subject.render(:not_a_string,[{:filename=>"file.mp3",:start=>0}]) }.to raise_error
      end
      
      it "should raise an exception when second argument array has a non-hash" do
        expect { subject.render("String",[:no,:hashes,:here]) }.to raise_error
      end
    end

#    context "given an empty list" do
#      pending
#    end

#    context "given a list of filenames" do
#      pending
#    end
  end

#  it "should have tests" do
#    pending "write tests or I will kneecap you"
#  end 
end 
