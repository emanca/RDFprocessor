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

# ROOT.gInterpreter.Declare("""
# #include <boost/histogram.hpp>
# using namespace boost::histogram;
# using boost_histogram = boost::histogram::histogram<std::vector<boost::histogram::axis::variable<> >, boost::histogram::storage_adaptor<std::vector<boost::histogram::accumulators::weighted_sum<>, std::allocator<boost::histogram::accumulators::weighted_sum<> > > > >;

# std::vector<std::vector<float>> convert(boost_histogram& h){ 
#     std::vector<float> vals;
#     std::vector<float> sumw2;
#     for (auto&& x : indexed(h)) {
#         const auto n = x->value();
#         const auto w2 = x->variance();
#         vals.emplace_back(n);
#         sumw2.emplace_back(w2);
#         }
#     return std::vector<std::vector<float>>{vals,sumw2};
#     }; 
# """
# )


