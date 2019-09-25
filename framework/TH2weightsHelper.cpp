#include "TH2weightsHelper.h"
   /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
   /// the columns which will be used.
   TH2weightsHelper::TH2weightsHelper(std::string name, std::string title, 
                    int nbinsX, std::vector<float> xbins,
                    int nbinsY, std::vector<float> ybins,
                    std::vector<std::string> weightNames
                    )
   {
      _name = name;
      _nbinsX = nbinsX;
      _xbins = xbins;
      _nbinsY = nbinsY;
      _ybins = ybins;
      _weightNames = weightNames;

      const auto nSlots = ROOT::IsImplicitMTEnabled() ? ROOT::GetImplicitMTPoolSize() : 1;
      for (auto i : ROOT::TSeqU(nSlots)) {
         fHistos.emplace_back(std::make_shared<std::vector<std::unique_ptr<TH2D>>>());
         (void)i;
      }
   }
  
   std::shared_ptr<std::vector<std::unique_ptr<TH2D>>> TH2weightsHelper::GetResultPtr() const { return fHistos[0]; }
   void TH2weightsHelper::Initialize() {}
   void TH2weightsHelper::InitTask(TTreeReader *, unsigned int) {}
   /// This is a method executed at every entry

   void TH2weightsHelper::Exec(unsigned int slot, const float &var1, const float &var2, const  ROOT::VecOps::RVec<float> &weights)
{
    
    std::vector<std::unique_ptr<TH2D>>& histos = *fHistos[slot];
    auto n_histos = weights.size();

    if (histos.empty()) {

      for (unsigned int i = 0; i < n_histos; ++i){
        std::unique_ptr<TH2D> h (new TH2D(std::string(_name+"_"+_weightNames[i]).c_str(), 
                              std::string(_name+"_"+_weightNames[i]).c_str(), 
                              _nbinsX, _xbins.data(), 
                              _nbinsY, _ybins.data()));
        h->SetDirectory(nullptr);
        histos.push_back(std::move(h));
      }
    }
    
    for (unsigned int i = 0; i < n_histos; ++i)
      histos[i]->Fill(var1, var2, weights[i]);
}
   /// This method is called at the end of the event loop. It is used to merge all the internal THnTs which
   /// were used in each of the data processing slots.
   void TH2weightsHelper::Finalize()
   {
      auto &res_vec = *fHistos[0];
      for (auto slot : ROOT::TSeqU(1, fHistos.size())) {
         auto& histo_vec = *fHistos[slot];
         for (auto i : ROOT::TSeqU(0, res_vec.size()))
           res_vec[i]->Add(histo_vec[i].get());
      }
   }
   std::string GetActionName(){
      return "TH2weightsHelper";
   }
