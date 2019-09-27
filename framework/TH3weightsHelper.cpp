#include "TH3weightsHelper.h"
   /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
   /// the columns which will be used.
   TH3weightsHelper::TH3weightsHelper(std::string name, std::string title, 
                    int nbinsX, std::vector<float> xbins,
                    int nbinsY, std::vector<float> ybins,
                    int nbinsZ, std::vector<float> zbins,
                    std::vector<std::string> weightNames
                    )
   {
      _name = name;
      _nbinsX = nbinsX;
      _xbins = xbins;
      _nbinsY = nbinsY;
      _ybins = ybins;
      _nbinsZ = nbinsZ;
      _zbins = zbins;
      _weightNames = weightNames;

      const auto nSlots = ROOT::IsImplicitMTEnabled() ? ROOT::GetImplicitMTPoolSize() : 1;
      for (auto i : ROOT::TSeqU(nSlots)) {
         fHistos.emplace_back(std::make_shared<std::vector<std::unique_ptr<TH3D>>>());
         (void)i;
      }
   }
  
   std::shared_ptr<std::vector<std::unique_ptr<TH3D>>> TH3weightsHelper::GetResultPtr() const { return fHistos[0]; }
   void TH3weightsHelper::Initialize() {}
   void TH3weightsHelper::InitTask(TTreeReader *, unsigned int) {}
   /// This is a method executed at every entry

   void TH3weightsHelper::Exec(unsigned int slot, const float &var1, const float &var2, const float &var3, const  ROOT::VecOps::RVec<float> &weights)
{
    
    std::vector<std::unique_ptr<TH3D>>& histos = *fHistos[slot];
    auto n_histos = weights.size();

    if (histos.empty()) {

      for (unsigned int i = 0; i < n_histos; ++i){
        std::unique_ptr<TH3D> h (new TH3D(std::string(_name+"_"+_weightNames[i]).c_str(), 
                              std::string(_name+"_"+_weightNames[i]).c_str(), 
                              _nbinsX, _xbins.data(), 
                              _nbinsY, _ybins.data(),
                              _nbinsZ, _zbins.data()));
        h->SetDirectory(nullptr);
        histos.push_back(std::move(h));
      }
    }
    
    for (unsigned int i = 0; i < n_histos; ++i)
      histos[i]->Fill(var1, var2, var3, weights[i]);
}
   /// This method is called at the end of the event loop. It is used to merge all the internal THnTs which
   /// were used in each of the data processing slots.
   void TH3weightsHelper::Finalize()
   {
      auto &res_vec = *fHistos[0];
      for (auto slot : ROOT::TSeqU(1, fHistos.size())) {
         auto& histo_vec = *fHistos[slot];
         for (auto i : ROOT::TSeqU(0, res_vec.size()))
           res_vec[i]->Add(histo_vec[i].get());
      }
   }
   std::string GetActionName(){
      return "TH3weightsHelper";
   }
