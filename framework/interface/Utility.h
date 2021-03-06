#ifndef UTILITY_H
#define UTILITY_H
#include <vector>
#include <boost/histogram.hpp>
#include "thread_safe_doubles.hpp"
using namespace boost::histogram;
// using boost_histogram = boost::histogram::histogram<std::vector<boost::histogram::axis::variable<> >, boost::histogram::storage_adaptor<std::vector<boost::histogram::accumulators::weighted_sum<>, std::allocator<boost::histogram::accumulators::weighted_sum<> > > > >;
using boost_histogram = boost::histogram::histogram<std::vector<boost::histogram::axis::variable<double, boost::use_default,boost::use_default, std::allocator<double>>,std::allocator<boost::histogram::axis::variable<double, boost::use_default,boost::use_default, std::allocator<double>>>>,boost::histogram::storage_adaptor<std::vector<boost::histogram::accumulators::thread_safe_doubles<double>,std::allocator<boost::histogram::accumulators::thread_safe_doubles<double>>>>>;

// std::vector<std::vector<float>> convert(boost_histogram& h){ 
//     std::vector<float> vals;
//     std::vector<float> sumw2;
//     for (auto&& x : indexed(h)) {
//         const auto v = *x;
//         // const auto n = x->value().load();
//         // const auto w2 = x->variance().load();
//         // std::cout<< n <<std::endl;
//         // vals.emplace_back(n);
//         // sumw2.emplace_back(w2);
//         }
//     return std::vector<std::vector<float>>{vals,sumw2};
// }

std::vector<float> convert(boost_histogram &h)
{
    std::vector<float> vals;
    for (auto &&x : indexed(h))
    {
        const auto n = *x;
        // const auto n = x->value().load();
        // const auto w2 = x->variance().load();
        // std::cout<< n <<std::endl;
        vals.emplace_back(n);
        // sumw2.emplace_back(w2);
    }
    return std::vector<float>{vals};
}
#endif


