#ifndef UTILITY_H
#define UTILITY_H
#include <vector>
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
} 
#endif


