#ifndef MODULE_H
#define MODULE_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "ROOT/RDF/RInterface.hxx"
#include "boost/rank_mod.hpp"
#include <boost/histogram.hpp>
#include <boost/functional/hash.hpp>
#include <memory>
#include <tuple>

using namespace ROOT::VecOps;
using RNode = ROOT::RDF::RNode;

template <typename T>
class Module
{

private:
public:
  virtual ~Module(){};
  virtual RNode run(RNode d) = 0;

  std::vector<ROOT::RDF::RResultPtr<T>> _container;
  //keep track of systematic variations
  std::map<std::pair<std::string, bool>, std::vector<std::string>> _variationRules; //std::map<std::pair<column,isWeight>, std::vector<variation_name>>

  std::vector<ROOT::RDF::RResultPtr<T>> get(){
    return _container;
  };

  void reset(){
    _container.clear(); 
  };
  
  void vary(std::string Column, bool isWeight, std::vector<std::string> variations)
  {
    auto pair = std::make_pair(Column, isWeight);
    _variationRules.insert(std::make_pair(pair, variations));
  }

  void setVariationRules(std::map<std::pair<std::string, bool>, std::vector<std::string>> variationRules)
  {
    _variationRules = variationRules;
  }

  std::map<std::pair<std::string, bool>, std::vector<std::string>> getVariationRules()
  {
    return _variationRules;
  }
};

#endif
