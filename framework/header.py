# Built-in/Generic Imports
import os
import sys
import time
import ROOT

def getValueType(obj):
    
    class_name = type(obj).__cpp_name__
    open_br, close_br = class_name.find('<'), class_name.rfind('>')
    value_type = class_name[open_br+1:close_br]
    return value_type


ROOT.gInterpreter.Declare("""
#include "/scratchnvme/emanca/wproperties-analysis/RDFprocessor/framework/interface/boostHistoHelper.hpp"
using RNode = ROOT::RDF::RNode;

template <typename... Ts>
struct Histogram
{
   ROOT::RDF::RResultPtr<std::map<std::string, boost_histogram>> operator()(RNode d, std::string name, std::map<std::pair<std::string, bool>, std::vector<std::string>> variationRules, std::vector<std::vector<float>> bins, const std::vector<std::string> & columns) 
   {
    auto vec = [](float value) {
        ROOT::VecOps::RVec<float> myvec;
        myvec.push_back(value);
        return myvec;
    };

    bool check = false;
    std::vector<std::string> new_cols;
    std::string s = "RVec";
    for(auto &col:columns){
        if(d.GetColumnType(col).find(s) != std::string::npos){// if it's a vector
            bool found = false;
            for(auto &x:variationRules){
                if(x.first.first==col) found = true;
            }
            if(found) check = true;
            new_cols.emplace_back(col);
        }
        else{// if it's a scalar
            bool found = false;
            for(auto &x:variationRules){
                if(x.first.first==col) found = true;
            }
            if(found) check = false;
            else{
                d = d.Define(Form("vec_%s",col.c_str()),vec,{Form("%s",col.c_str())});
                new_cols.emplace_back(Form("vec_%s",col.c_str()));
            }
        }
    }
    boostHistoHelper helper(name, new_cols, variationRules, bins);
    auto h = d.Book<ROOT::VecOps::RVec<Ts>...>(std::move(helper), new_cols);
    return h;
    }
};
"""
)

ROOT.gInterpreter.Declare("""
#include <boost/histogram.hpp>
using namespace boost::histogram;
using boost_histogram = boost::histogram::histogram<std::vector<boost::histogram::axis::variable<> >, boost::histogram::storage_adaptor<std::vector<boost::histogram::accumulators::weighted_sum<>, std::allocator<boost::histogram::accumulators::weighted_sum<> > > > >;

std::vector<std::vector<float>> convert(boost_histogram& h){ 
    std::vector<float> vals;
    std::vector<float> sumw2;
    for (auto&& x : indexed(h)) {
        const auto n = x->value();
        const auto w2 = x->variance();
        //std::cout<< n <<std::endl;
        vals.emplace_back(n);
        sumw2.emplace_back(w2);
        }
    return std::vector<std::vector<float>>{vals,sumw2};
    }; 
"""
)


