#ifndef DATAFORMAT_H
#define DATAFORMAT_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "ROOT/RDF/RInterface.hxx"
#include <boost/histogram.hpp>
#include "boostHistoHelper.hpp"
#include <tuple>
#include <vector>
#include <string>
#include <iostream>
using namespace ROOT::VecOps;
using RNode = ROOT::RDF::RNode;

template <std::size_t Ncols, std::size_t Nweights, typename... Ts>
struct Histogram
{
    ROOT::RDF::RResultPtr<std::map<std::string, boost_histogram>> operator()(RNode d, std::string name, std::vector<std::vector<float>> bins, const std::vector<std::string> &columns, std::vector<std::vector<std::string>> variationRules)
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
            // std::cout << col << " " << d.GetColumnType(col) << std::endl;
            if (d.GetColumnType(col).find(s) != std::string::npos)
            { // if it's a vector
                // std::cout << "it's a vector" << std::endl;
                new_cols.emplace_back(col);
            }
            else
            { // if it's a scalar
                d = d.Define(Form("vec_%s", col.c_str()), vec, {Form("%s", col.c_str())});
                new_cols.emplace_back(Form("vec_%s", col.c_str()));
            }
            index++;
        }
        // std::cout<< "processing..." <<std::endl;
        // for(auto &col:new_cols) std::cout<< col <<std::endl;
        // std::cout<< "call helper..." <<std::endl;
        boostHistoHelper<Ncols, Nweights> helper(name, variationRules, bins);
        // std::cout<< "call book..." <<std::endl;
        auto h = d.Book<ROOT::VecOps::RVec<Ts>...>(std::move(helper), new_cols);
        return h;
    }
};

#endif

