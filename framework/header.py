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

template <std::size_t N,typename... Ts>
struct Histogram
{
    ROOT::RDF::RResultPtr<std::map<std::string, boost_histogram>> operator()(RNode d, std::string name, std::map<std::pair<std::string, bool>, std::vector<std::string>> variationRules, std::vector<std::vector<float>> bins, const std::vector<std::string> &columns, const int &n_weights)
    {
        auto vec = [](float value) {
            ROOT::VecOps::RVec<float> myvec;
            myvec.push_back(value);
            return myvec;
        };

        bool check = false;
        std::vector<std::string> new_cols;
        std::string s = "RVec";
        int index = 0;
        for (auto &col : columns)
        {
            std::cout<< col <<std::endl;
            if (d.GetColumnType(col).find(s) != std::string::npos)
            { // if it's a vector
                bool found = false;
                for (auto &x : variationRules)
                {
                    if (x.first.first == col)
                        found = true;
                }
                if (found)
                    check = true;
                new_cols.emplace_back(col);
            }
            else
            { // if it's a scalar
                bool found = false;
                for (auto &x : variationRules)
                {
                    if (x.first.first == col)
                        found = true;
                }
                if (found)
                    check = false;
                else
                {
                    //check if it's a weight
                    if (index >= (columns.size() - n_weights))
                    {
                        std::cout << "this column is a weight: " << col << std::endl;
                        auto pair = std::make_pair("vec_"+col, true);
                        variationRules.insert(std::make_pair(pair, std::vector<std::string>({""})));
                    }
                    d = d.Define(Form("vec_%s", col.c_str()), vec, {Form("%s", col.c_str())});
                    new_cols.emplace_back(Form("vec_%s", col.c_str()));
                }
            }
            index++;
        }
        //std::cout<< "processing..." <<std::endl;
        //for(auto &col:new_cols) std::cout<< col <<std::endl;
        //std::cout<< "call helper..." <<std::endl;
        boostHistoHelper<N> helper(name, new_cols, variationRules, bins);
        //std::cout<< "call book..." <<std::endl;
        auto h = d.Book<ROOT::VecOps::RVec<Ts>...>(std::move(helper), new_cols);
        return h;
    }
};
"""
)

ROOT.gInterpreter.Declare("""
#include "/scratchnvme/emanca/wproperties-analysis/RDFprocessor/framework/interface/boostProfHelper.hpp"
using RNode = ROOT::RDF::RNode;

template <std::size_t N,typename... Ts>
struct Profile
{
    ROOT::RDF::RResultPtr<std::map<std::string, boost_histogram_prof>> operator()(RNode d, std::string name, std::map<std::pair<std::string, bool>, std::vector<std::string>> variationRules, std::vector<std::vector<float>> bins, const std::vector<std::string> &columns, const int &n_weights)
    {
        auto vec = [](float value) {
            ROOT::VecOps::RVec<float> myvec;
            myvec.push_back(value);
            return myvec;
        };

        bool check = false;
        std::vector<std::string> new_cols;
        std::string s = "RVec";
        int index = 0;
        for (auto &col : columns)
        {
            std::cout<< col << std::endl;
            if (d.GetColumnType(col).find(s) != std::string::npos)
            { // if it's a vector
                bool found = false;
                for (auto &x : variationRules)
                {
                    if (x.first.first == col)
                        found = true;
                }
                if (found)
                    check = true;
                new_cols.emplace_back(col);
            }
            else
            { // if it's a scalar
                bool found = false;
                for (auto &x : variationRules)
                {
                    if (x.first.first == col)
                        found = true;
                }
                if (found)
                    check = false;
                else
                {
                    //check if it's a weight
                    if (index >= (columns.size() - n_weights))
                    {
                        std::cout << "this column is a weight: " << col << std::endl;
                        auto pair = std::make_pair("vec_"+col, true);
                        variationRules.insert(std::make_pair(pair, std::vector<std::string>({""})));
                    }
                    d = d.Define(Form("vec_%s", col.c_str()), vec, {Form("%s", col.c_str())});
                    new_cols.emplace_back(Form("vec_%s", col.c_str()));
                }
            }
            index++;
        }
        // for(auto &var:variationRules) std::cout<< var.first.first <<std::endl;
        boostProfHelper<N> helper(name, new_cols, variationRules, bins);
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


