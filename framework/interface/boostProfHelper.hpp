#ifndef BOOSTPROFHELPER_H
#define BOOSTPROFHELPER_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "ROOT/RDF/RInterface.hxx"
#include <boost/histogram.hpp>
#include <boost/format.hpp> // only needed for printing
#include <boost/functional/hash.hpp>
#include <memory>
#include <tuple>

// template <std::size_t... Is>
// auto create_tuple_impl(std::index_sequence<Is...>, const std::vector<double> &arguments)
// {
//     return std::make_tuple(arguments[Is]...);
// }

// template <std::size_t N>
// auto create_tuple(const std::vector<double> &arguments)
// {
//     return create_tuple_impl(std::make_index_sequence<N>{}, arguments);
// }
using boost_histogram_prof = boost::histogram::histogram<std::vector<boost::histogram::axis::variable<>>, boost::histogram::storage_adaptor<std::vector<boost::histogram::accumulators::weighted_mean<>, std::allocator<boost::histogram::accumulators::weighted_mean<>>>>>;

template <std::size_t N>
class boostProfHelper : public ROOT::Detail::RDF::RActionImpl<boostProfHelper<N>>
{

public:
    /// This type is a requirement for every helper.
    using Result_t = std::map<std::string, boost_histogram_prof>;
private:
    std::vector<std::shared_ptr<std::map<std::string, boost_histogram_prof>>> fHistos;     // one per data processing slot
    std::map<std::pair<std::string, bool>, std::vector<std::string>> _variationRules; //to keep track of the ordering
    std::string _name;
    std::vector<std::string> _columns;
    std::vector<boost::histogram::axis::variable<>> _v;

public:
    /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
    /// the columns which will be used.
    boostProfHelper(std::string name,
                     std::vector<std::string> columns,
                     std::map<std::pair<std::string, bool>, std::vector<std::string>> variationRules,
                     std::vector<std::vector<float>> bins)
    {
        _name = name;
        _columns = columns;
        _columns.erase(_columns.begin());
        _variationRules = variationRules;
        for (auto &b : bins)
            _v.emplace_back(b);

        const auto nSlots = ROOT::IsImplicitMTEnabled() ? ROOT::GetThreadPoolSize() : 1;

        for (auto slot : ROOT::TSeqU(nSlots))
        {
            fHistos.emplace_back(std::make_shared<std::map<std::string, boost_histogram_prof>>());
            (void)slot;

            std::map<std::string, boost_histogram_prof> &hmap = *fHistos[slot];

            std::string slotnum = "";
            slotnum = slot > 0 ? std::to_string(slot) : "";
            // first make nominal histogram
            auto htmp = boost::histogram::make_weighted_profile(_v);
            // std::cout << "rank is " << htmp.rank() << std::endl;
            hmap.insert(std::make_pair(_name, htmp));
            //then check if any variation
            for (auto &x : _variationRules)
            {
                // std::cout << x.first.first << " beg" <<std::endl;
                int icol = getIndex(_columns, x.first.first);
                // std::cout << icol << std::endl;
                if (icol < 0)
                    continue;
                auto htmp = boost::histogram::make_weighted_profile(_v);
                auto names = x.second;
                for (auto &n : names)
                {
                    if (n == "")
                        continue;
                    std::string histoname = _name + "_" + n;
                    // std::cout << "histoname " << histoname << std::endl;
                    hmap.insert(std::make_pair(histoname, htmp));
                }
            }
        }
    }

    boostProfHelper(boostProfHelper &&) = default;
    boostProfHelper(const boostProfHelper &) = delete;
    std::shared_ptr<std::map<std::string, boost_histogram_prof>> GetResultPtr() const { return fHistos[0]; }
    void Initialize() {}
    void InitTask(TTreeReader *, unsigned int) {}
    /// This is a method executed at every entry

    int getIndex(std::vector<std::string> v, std::string K)
    {
        auto it = std::find(v.begin(), v.end(), K);
        
        if (it != v.end())
        {
            int index = std::distance(v.begin(), it);
            return index;
        }
        else
        {
            return -1;
        }
    }

    template <typename... Ts>
    void Exec(unsigned int slot, ROOT::VecOps::RVec<Ts>... vecs)
    {
        // std::cout<<"exec"<<std::endl;
        std::map<std::string, boost_histogram_prof> &hmap = *fHistos[slot];
        std::vector<ROOT::VecOps::RVec<double>> matrix;
        (matrix.push_back(vecs), ...);
        float toProfile = matrix[0][0];
        // std::cout << toProfile << std::endl;
        matrix.erase(matrix.begin());
        
        for (auto &x : hmap)
        {
            //find variation linked to this histogram
            std::string delimiter = "_";
            size_t pos = 0;
            std::string s = x.first;
            std::string token;
            while ((pos = s.find(delimiter)) != std::string::npos)
            {
                token = s.substr(0, pos);
                // std::cout << "token " << token << std::endl;
                s.erase(0, pos + delimiter.length());
                // std::cout<< "string " << s << std::endl;
            }
            
            //get indices
            std::vector<std::pair<int, bool>> indices;
            indices.resize(sizeof...(vecs)-1);
            for (auto &col : _variationRules)
            {
                //which column are you?
                int icol = getIndex(_columns, col.first.first);
                if (icol < 0)
                    continue;
                // std::cout << icol << " " << s <<std::endl;
                int i = getIndex(col.second, s) + 1; //find variation
                indices[icol] = std::make_pair(i, col.first.second);
                // std::cout<< col.first.first << " index is " << i << std::endl;
            }
            //extract values from columns
            std::vector<double> values;
            double weight = 1.;
            // for(auto &idx:indices) std::cout<< idx.first <<std::endl;
            for (unsigned int i = 0; i < indices.size(); i++)
            {
                if (!(indices[i].second)){
                    // std::cout << "extract values " << i << " " << weight << " " << matrix[i][indices[i].first] << std::endl;
                    values.push_back(matrix[i][indices[i].first]);
                }
                else
                {
                    // std::cout << "extract values " << i << " " << weight << " " << matrix[i][indices[i].first] << std::endl;
                    weight *= matrix[i][indices[i].first];
                }
            }
            auto t = create_tuple<N>(values);
            // std::cout
            //     << "(" << std::get<0>(t) << ", " << std::get<1>(t)
            //     << ")\n";
            // std::cout<< weight << "weight" << std::endl;
            std::apply([&](auto &&... args) { x.second(boost::histogram::sample(toProfile), boost::histogram::weight(1.), args...); }, t);
            // std::apply([&](auto &&... args) { x.second(boost::histogram::sample(toProfile), boost::histogram::weight(weight), args...); }, t);
            // std::cout << "end exec" << std::endl;
        }
    }
    void Finalize()
    {
        std::cout << "in Finalize" << std::endl;
        auto &res = *fHistos[0];
        for (auto slot : ROOT::TSeqU(1, fHistos.size()))
        {
            auto &map = *fHistos[slot];
            for (auto &x : res)
            {
                x.second += map[x.first];
                for (auto &&h : indexed(x.second))
                {
                    const auto n = h->value();
                    const auto w2 = h->variance();
                }
            }
        }
    }
    std::string GetActionName()
    {
        return "boostProfHelper";
    }
};

#endif
