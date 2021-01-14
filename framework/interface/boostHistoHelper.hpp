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

template <std::size_t N>
class boostHistoHelper : public ROOT::Detail::RDF::RActionImpl<boostHistoHelper<N>>
{

public:
   /// This type is a requirement for every helper.
   using Result_t = std::map<std::string, boost_histogram>;

private:
   std::vector<std::shared_ptr<std::map<std::string, boost_histogram>>> fHistos;     // one per data processing slot
   std::map<std::pair<std::string, bool>, std::vector<std::string>> _variationRules; //to keep track of the ordering
   std::string _name;
   std::vector<std::string> _columns;
   std::vector<boost::histogram::axis::variable<>> _v;

public:
   /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
   /// the columns which will be used.
   boostHistoHelper(std::string name,
                    std::vector<std::string> columns,
                    std::map<std::pair<std::string, bool>, std::vector<std::string>> variationRules,
                    std::vector<std::vector<float>> bins)
   {
      _name = name;
      _columns = columns;
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
         for (auto &x : _variationRules)
         {
            int icol = getIndex(_columns, x.first.first);
            // std::cout << icol << std::endl;
            if (icol < 0)
               continue;
            auto htmp = boost::histogram::make_weighted_histogram(_v);
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

   boostHistoHelper(boostHistoHelper &&) = default;
   boostHistoHelper(const boostHistoHelper &) = delete;
   std::shared_ptr<std::map<std::string, boost_histogram>> GetResultPtr() const { return fHistos[0]; }
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
      std::map<std::string, boost_histogram> &hmap = *fHistos[slot];
      std::vector<ROOT::VecOps::RVec<double>> matrix;
      (matrix.push_back(vecs), ...);

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
         indices.resize(sizeof...(vecs));
         for (auto &col : _variationRules)
         {
            //which column are you?
            int icol = getIndex(_columns, col.first.first); // find variated column in column list
            // std::cout << col.first.first << " " << icol << " " << s << std::endl;
            if (icol < 0)
               continue;                                            // this column doesn't enter in this histogram
            else if (col.second.size() == 1)                        // if it's a fake variation with 1 element only
               indices[icol] = std::make_pair(0, col.first.second); 
            else
            {
               if (s == _name)
                  indices[icol] = std::make_pair(0, col.first.second);
               else
               {
                  int i = getIndex(col.second, s); // assign variation to the right histogram
                  indices[icol] = std::make_pair(i, col.first.second);
               }
            }
            // std::cout << col.first.first << " index is " << indices[icol].first << std::endl;
         }
         //extract values from columns
         std::vector<double> values;
         double weight = 1.;

         for (unsigned int i = 0; i < indices.size(); i++)
         {
            // std::cout << indices[i].first << " " << indices[i].second << std::endl;
            if (!(indices[i].second))
               values.push_back(matrix[i][indices[i].first]);
            else
            {
               // std::cout << i << " " << weight << " " << matrix[i][indices[i].first] << std::endl;
               weight *= matrix[i][indices[i].first];
            }
         }
         auto t = create_tuple<N>(values);
         // std::cout
         //     << "(" << std::get<0>(t) << ", " << std::get<1>(t)
         //     << ", " << std::get<2>(t) << ", " << std::get<3>(t) << ", " << std::get<4>(t) << ")\n";
         std::apply([&](auto &&... args) { x.second(boost::histogram::weight(weight), args...); }, t);
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
            x.second += map[x.first];
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
