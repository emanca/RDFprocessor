#ifndef BOOSTHISTOHELPER_H
#define BOOSTHISTOHELPER_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "ROOT/RDF/RInterface.hxx"
#include <boost/histogram.hpp>
#include <boost/format.hpp> // only needed for printing
#include <boost/functional/hash.hpp>
#include <memory>
#include <tuple>

template <std::size_t... Is>
auto create_tuple_impl(std::index_sequence<Is...>, const std::vector<double> &arguments)
{
   return std::make_tuple(arguments[Is]...);
}

template <std::size_t N>
auto create_tuple(const std::vector<double> &arguments)
{
   return create_tuple_impl(std::make_index_sequence<N>{}, arguments);
}

using boost_histogram = boost::histogram::histogram<std::vector<boost::histogram::axis::variable<>>, boost::histogram::storage_adaptor<std::vector<boost::histogram::accumulators::weighted_sum<>, std::allocator<boost::histogram::accumulators::weighted_sum<>>>>>;

template <std::size_t Ncols, std::size_t Nweights>
class boostHistoHelper : public ROOT::Detail::RDF::RActionImpl<boostHistoHelper<Ncols, Nweights>>
{

public:
   /// This type is a requirement for every helper.
   using Result_t = std::map<std::string, boost_histogram>;

private:
   std::vector<std::shared_ptr<std::map<std::string, boost_histogram>>> fHistos; // one per data processing slot
   std::vector<std::vector<std::string>> _variationRules;                        //to keep track of the variations --> ordered as columns
   std::string _name;
   std::vector<boost::histogram::axis::variable<>> _v;

public:
   /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
   /// the columns which will be used.
   boostHistoHelper(std::string name,
                    std::vector<std::vector<std::string>> variationRules,
                    std::vector<std::vector<float>> bins)
   {
      
      _name = name;
      _variationRules = variationRules;

      for (auto &b : bins)
         _v.emplace_back(b);
      
      const auto nSlots = ROOT::IsImplicitMTEnabled() ? ROOT::GetThreadPoolSize() : 1;
      
      for (auto slot : ROOT::TSeqU(nSlots))
      {
         fHistos.emplace_back(std::make_shared<std::map<std::string, boost_histogram>>());
         (void)slot;

         std::map<std::string, boost_histogram> &hmap = *fHistos[slot];

         std::string slotnum = "";
         slotnum = slot > 0 ? std::to_string(slot) : "";
         // first make nominal histogram
         auto htmp = boost::histogram::make_weighted_histogram(_v);
         // std::cout << "rank is " << htmp.rank() << std::endl;
         hmap.insert(std::make_pair(_name, htmp));
         //then check if variations are asked
         for (auto &groupOfVars : _variationRules)
         {
            if (groupOfVars[0] == "")
               continue;
            for (auto &var : groupOfVars)
            {
               auto htmp = boost::histogram::make_weighted_histogram(_v);
               std::string histoname = _name + "_" + var;
               // std::cout << "histoname " << histoname << std::endl;
               hmap.insert(std::make_pair(histoname, htmp));
            }
         }
      }
   }

   boostHistoHelper(boostHistoHelper &&) = default;
   boostHistoHelper(const boostHistoHelper &) = delete;
   std::shared_ptr<std::map<std::string, boost_histogram>> GetResultPtr() const { return fHistos[0]; }
   void Initialize() {}
   void InitTask(TTreeReader *, unsigned int) {}
   /// This is a method executed at every entry

   template <typename... Ts>
   void Exec(unsigned int slot, Ts... cols)
   {
      // std::cout << "exec" << std::endl;
      std::map<std::string, boost_histogram> &hmap = *fHistos[slot];
      //fill nominal histogram
      //extract values from columns
      std::vector<ROOT::VecOps::RVec<double>> values;
      (values.push_back(cols), ...); // this contains all the columns

      //find a way to simplify this
      // columns
      std::vector<double> columns;
      for (unsigned int i = 0; i < Ncols; i++)
      {
         columns.push_back(values[i][0]);
         // std::cout << "columns: " << values[i][0] << std::endl;
      }
      // weights
      std::vector<double> weights;
      for (unsigned int i = Ncols; i < Ncols + Nweights; i++)
      {
         weights.push_back(values[i][0]);
         // std::cout << "weights " << values[i][0] << std::endl;
      }
      double weight = std::accumulate(std::begin(weights), std::end(weights), 1., std::multiplies<double>());
      // std::cout<< "final weight " << weight << std::endl;
      // variations
      std::vector<ROOT::VecOps::RVec<double>> variationVecs(values.begin() + Ncols + Nweights, values.end());
      // std::cout << variationVecs.size() << " size " << std::endl;
      auto t = create_tuple<Ncols>(columns);
      // std::cout
      //     << "(" << std::get<0>(t) << ", " << std::get<1>(t)
      //     << ", " << std::get<2>(t) << ", " << std::get<3>(t) << ", " << std::get<4>(t) << ")\n";
      std::apply([&](auto &&...args) { hmap.at(_name)(boost::histogram::weight(weight), args...); }, t);

      // now fill variations
      for (unsigned int i = 0; i < _variationRules.size(); i++)
      {
         // this index will tell which column to vary
         // std::cout << " variations size" << _variationRules.size() << std::endl;
         if (_variationRules[i][0] == "")
         {
            auto it = variationVecs.begin() + i;
            auto newIt = variationVecs.insert(it, ROOT::VecOps::RVec<double>{-999999.}); // patch vector of variations
            continue;                                  // nothing to vary
         }
         for (unsigned int j = 0; j < _variationRules[i].size(); j++)
         {
            // std::cout << " variations number " << i << " " << _variationRules[i].size() << std::endl;
            // first copy the nominal vector every time
            std::vector<double> columns_var(columns);
            std::vector<double> weights_var(weights);
            // substitute the relevant column with its variation and fill
            if (i < (Ncols + Nweights))
            {
               if (i < Ncols) columns_var[i] = variationVecs[i][j];
               else
                  weights_var[i - Ncols] = variationVecs[i][j];
            }
            else
               throw std::invalid_argument("you're trying to vary a variation...");
            auto t = create_tuple<Ncols>(columns_var);
            double weight = std::accumulate(std::begin(weights), std::end(weights), 1., std::multiplies<double>());
            std::string histoname = _name + "_" + _variationRules[i][j];
            std::apply([&](auto &&...args) { hmap.at(histoname)(boost::histogram::weight(weight), args...); }, t);
         }
      }
   }

   void Finalize()
   {
      // std::cout << "in Finalize" << std::endl;
      auto &res = *fHistos[0];
      for (auto slot : ROOT::TSeqU(1, fHistos.size()))
      {
         auto &map = *fHistos[slot];
         for (auto &x : res)
         {
            x.second += map.at(x.first);
            // std::cout << x.second.rank() << std::endl;
         }
      }
   }
   std::string GetActionName()
   {
      return "boostHistoHelper";
   }
};

#endif
