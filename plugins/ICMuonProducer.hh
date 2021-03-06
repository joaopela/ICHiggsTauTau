#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "UserCode/ICHiggsTauTau/interface/Muon.hh"

// #include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

//#include "Muon/MuonAnalysisTools/interface/MuonMVAEstimator.h"


class ICMuonProducer : public edm::EDProducer {
   public:
      explicit ICMuonProducer(const edm::ParameterSet&);
      ~ICMuonProducer();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      virtual void beginJob() ;
      virtual void produce(edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
      
      virtual void beginRun(edm::Run&, edm::EventSetup const&);
      virtual void endRun(edm::Run&, edm::EventSetup const&);
      virtual void beginLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&);
      virtual void endLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&);

      // virtual bool electronIDForMVA(reco::GsfElectron const& elec, reco::Vertex const& vtx);
      // virtual bool muonIDForMVA(reco::Muon const& muon);

      // ----------member data ---------------------------
      std::vector<ic::Muon> *muons_;
      edm::InputTag input_;
      std::string branch_name_;
      std::string pfiso_postfix_;
      edm::InputTag vertex_input_;
      bool is_pf_;
      std::map<std::string, std::size_t> observed_idiso_;
      double min_pt_;
      double max_eta_;
};