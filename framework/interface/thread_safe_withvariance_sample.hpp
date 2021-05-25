// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ACCUMULATORS_THREAD_SAFE_WITHVARIANCE_SAMPLE_HPP
#define BOOST_HISTOGRAM_ACCUMULATORS_THREAD_SAFE_WITHVARIANCE_SAMPLE_HPP

#include <atomic>
#include <boost/core/nvp.hpp>
#include <boost/mp11/utility.hpp>
#include <type_traits>
#include <vector>
#include <boost/histogram/fwd.hpp> // for weighted_sum<>

namespace boost
{
    namespace histogram
    {
        namespace accumulators
        {

            template <class T, std::size_t D>
            class thread_safe_withvariance_sample
            {
            public:
                using value_type = T;
                using vec_value_type = std::vector<value_type>;
                using const_reference = const value_type &;
                using super_t = std::atomic<T>;
                using const_reference_atomic = const super_t &;

                // // initialize to 0
                // thread_safe_withvariance_sample() noexcept : thread_safe_withvariance_sample(super_t(static_cast<T>(0)), super_t(static_cast<T>(0))) {}
                // non-atomic copy and assign is allowed, because storage is locked in this case
                thread_safe_withvariance_sample() = default;
                // copy constructor
                thread_safe_withvariance_sample(const thread_safe_withvariance_sample &o) noexcept
                {
                    for (auto i = 0; i < D; i++)
                    {
                        sum_of_weights_[i]=(*(o.value())[i].load());
                        sum_of_weights_squared_[i]=(*(o.variance())[i].load());
                    }
                }

                thread_safe_withvariance_sample &operator=(const thread_safe_withvariance_sample &o) noexcept
                {
                    for (auto i = 0; i < D; i++)
                    {
                        sum_of_weights_[i].store(o.value()[i].load());
                        sum_of_weights_squared_[i].store(o.variance()[i].load());
                    }
                    return *this;
                }
                // initialize to value and variance
                thread_safe_withvariance_sample(vec_value_type value, vec_value_type variance) : sum_of_weights_(value), sum_of_weights_squared_(variance) {}
                // // initialize to weight type
                // template <class weightType>
                // thread_safe_withvariance_sample &operator=(const weight_type<weightType> &w)
                // {
                //     sum_of_weights_.store(w.value.load());
                //     sum_of_weights_squared_.store(w.variance.load());
                //     return *this;
                // }
                // add two thread safe objects
                thread_safe_withvariance_sample &operator+=(const thread_safe_withvariance_sample &arg)
                {
                    for (unsigned int i = 0; i < sum_of_weights_.size(); i++)
                    {
                        sum_of_weights_[i] += arg.sum_of_weights_[i].load();
                        sum_of_weights_squared_[i] += arg.sum_of_weights_squared_[i].load();
                        return *this;
                    }
                }

                thread_safe_withvariance_sample &operator+=(vec_value_type arg)
                {
                    for (unsigned int i = 0; i < sum_of_weights_.size(); i++)
                    {
                        // use Josh's trick for summing doubles
                        double old_val = sum_of_weights_[i].load();
                        double desired_val = old_val + arg[i];
                        while (!sum_of_weights_[i].compare_exchange_weak(old_val, desired_val))
                        {
                            desired_val = old_val + arg[i];
                        }

                        double old_variance = sum_of_weights_squared_[i].load();
                        double desired_variance = old_variance + arg[i] * arg[i];
                        while (!sum_of_weights_squared_[i].compare_exchange_weak(old_variance, desired_variance))
                        {
                            desired_variance = old_variance + arg[i] * arg[i];
                        }
                    }
                    return *this;
                }
                // Increment by one.
                thread_safe_withvariance_sample &operator++()
                {
                    for (unsigned int i = 0; i < sum_of_weights_.size(); i++)
                    {
                        // use Josh's trick for summing doubles
                        double old_val = sum_of_weights_.load();
                        double desired_val = old_val + 1.;
                        while (!sum_of_weights_.compare_exchange_weak(old_val, desired_val))
                        {
                            desired_val = old_val + 1.;
                        }

                        double old_variance = sum_of_weights_squared_.load();
                        double desired_variance = old_variance + 1. * 1.;
                        while (!sum_of_weights_squared_.compare_exchange_weak(old_variance, desired_variance))
                        {
                            desired_variance = old_variance + 1. * 1.;
                        }
                        // update the current object
                        sum_of_weights_.store(desired_val);
                        sum_of_weights_squared_.store(desired_variance);
                    }
                    return *this;
                }
                // add a basic type to thread safe objects
                template <class weightType>
                thread_safe_withvariance_sample &operator+=(const weight_type<weightType> &w)
                {
                    for (unsigned int i = 0; i < sum_of_weights_.size(); i++)
                    {
                        // use Josh's trick for summing doubles
                        double old_val = sum_of_weights_[i].load();
                        double desired_val = old_val + w.value;
                        while (!sum_of_weights_[i].compare_exchange_weak(old_val, desired_val))
                        {
                            desired_val = old_val + w.value;
                        }

                        double old_variance = sum_of_weights_squared_[i].load();
                        double desired_variance = old_variance + w.value * w.value;
                        while (!sum_of_weights_squared_[i].compare_exchange_weak(old_variance, desired_variance))
                        {
                            desired_variance = old_variance + w.value * w.value;
                        }
                    }
                    return *this;
                }

                
                /// Return value of the sum.
                std::unique_ptr<std::array<super_t, D>> value() const noexcept { return sum_of_weights_; }

                /// Return estimated variance of the sum.
                std::unique_ptr<std::array<super_t, D>> variance() const noexcept { return sum_of_weights_squared_; }

                // lossy conversion must be explicit
                explicit operator const_reference() const { return sum_of_weights_; }

            private:
                std::array<super_t, D> sum_of_weights_ = {};
                std::array<super_t, D> sum_of_weights_squared_ = {};
            };

        } // namespace accumulators
    }     // namespace histogram
} // namespace boost

#endif