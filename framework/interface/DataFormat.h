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
            // std::cout<< col <<std::endl;
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
                        // std::cout << "this column is a weight: " << col << std::endl;
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

#include "boostProfHelper.hpp"

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
            // std::cout<< col << std::endl;
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
        // // for(auto &var:variationRules) std::cout<< var.first.first <<std::endl;
        boostProfHelper<N> helper(name, new_cols, variationRules, bins);
        auto h = d.Book<ROOT::VecOps::RVec<Ts>...>(std::move(helper), new_cols);
        return h;
    }
};
#endif


