#include "TH1weightsHelper.h"
   /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
   /// the columns which will be used.
   TH1weightsHelper::TH1weightsHelper(std::string name, std::string title, 
                    int nbinsX, std::vector<float> xbins,
                    std::vector<std::string> weightNames
                    )
   {
      _name = name;
      _nbinsX = nbinsX;
      _xbins = xbins;
      _weightNames = weightNames;

      const auto nSlots = ROOT::IsImplicitMTEnabled() ? ROOT::GetImplicitMTPoolSize() : 1;
      for (auto i : ROOT::TSeqU(nSlots)) {
         fHistos.emplace_back(std::make_shared<std::vector<std::unique_ptr<TH1D>>>());
         (void)i;
      }
   }
  
   std::shared_ptr<std::vector<std::unique_ptr<TH1D>>> TH1weightsHelper::GetResultPtr() const { return fHistos[0]; }
   void TH1weightsHelper::Initialize() {}
   void TH1weightsHelper::InitTask(TTreeReader *, unsigned int) {}
   /// This is a method executed at every entry

   void TH1weightsHelper::Exec(unsigned int slot, const float &var1, const  ROOT::VecOps::RVec<float> &weights)
{
    
    std::vector<std::unique_ptr<TH1D>>& histos = *fHistos[slot];
    auto n_histos = weights.size();

    if (histos.empty()) {

      for (unsigned int i = 0; i < n_histos; ++i){
        std::unique_ptr<TH1D> h (new TH1D(std::string(_name+"_"+_weightNames[i]).c_str(), 
                              std::string(_name+"_"+_weightNames[i]).c_str(), 
                              _nbinsX, _xbins.data()));
        h->SetDirectory(nullptr);
        histos.push_back(std::move(h));
      }
    }
    
    for (unsigned int i = 0; i < n_histos; ++i)
      histos[i]->Fill(var1, weights[i]);
}
   /// This method is called at the end of the event loop. It is used to merge all the internal THnTs which
   /// were used in each of the data processing slots.
   void TH1weightsHelper::Finalize()
   {
      auto &res_vec = *fHistos[0];
      for (auto slot : ROOT::TSeqU(1, fHistos.size())) {
         auto& histo_vec = *fHistos[slot];
         for (auto i : ROOT::TSeqU(0, res_vec.size()))
           res_vec[i]->Add(histo_vec[i].get());
      }
   }
   std::string GetActionName(){
      return "TH1weightsHelper";
   }
