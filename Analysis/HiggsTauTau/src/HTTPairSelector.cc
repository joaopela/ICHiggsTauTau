#include "UserCode/ICHiggsTauTau/Analysis/HiggsTauTau/interface/HTTPairSelector.h"
#include "UserCode/ICHiggsTauTau/interface/PFJet.hh"
#include "UserCode/ICHiggsTauTau/Analysis/Utilities/interface/FnPredicates.h"
#include "UserCode/ICHiggsTauTau/Analysis/Utilities/interface/FnPairs.h"
#include <boost/functional/hash.hpp>
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/format.hpp"

namespace ic {

  HTTPairSelector::HTTPairSelector(std::string const& name) : ModuleBase(name), channel_(channel::et) {
    pair_label_ = "emtauCandidates";
    mva_met_from_vector_ = true;
    faked_tau_selector_ = 0;
    hadronic_tau_selector_ = 0;
    met_label_ = "pfMVAMet";
    fs_ = NULL;
    hists_.resize(1);
    use_most_isolated_ = false;
    scale_met_for_tau_ = 0;
    tau_scale_ = 1.0;
    allowed_tau_modes_ = "";
    gen_taus_label_ = "genParticlesTaus";
  }

  HTTPairSelector::~HTTPairSelector() {
    ;
  }

  int HTTPairSelector::PreAnalysis() {
    std::string param_fmt = "%-25s %-40s\n";

    std::cout << "-------------------------------------" << std::endl;
    std::cout << "HTTPairSelector" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    std::cout << boost::format(param_fmt) % "pair_label"            % pair_label_;
    std::cout << boost::format(param_fmt) % "met_label"             % met_label_;
    std::cout << boost::format(param_fmt) % "use_most_isolated"     % use_most_isolated_;
    std::cout << boost::format(param_fmt) % "scale_met_for_tau"     % scale_met_for_tau_;
    std::cout << boost::format(param_fmt) % "tau_scale"             % tau_scale_;
    std::string allowed_str = "";
    if (allowed_tau_modes_ == "") {
      allowed_str = "all modes";
    } else {
      std::vector<std::string> tau_mode_vec;
      boost::split(tau_mode_vec, allowed_tau_modes_, boost::is_any_of(","));
      for (unsigned i = 0; i < tau_mode_vec.size(); ++i) {
        int tau_mode = boost::lexical_cast<int>(tau_mode_vec[i]);
        tau_mode_set_.insert(tau_mode);
        allowed_str += (tau_mode_vec[i] + " ");
      } 
    }
    std::cout << boost::format(param_fmt) % "allowed_tau_modes"     % allowed_str;
    std::cout << boost::format(param_fmt) % "faked_tau_selector"    % faked_tau_selector_;
    std::cout << boost::format(param_fmt) % "hadronic_tau_selector" % hadronic_tau_selector_;
    std::cout << boost::format(param_fmt) % "gen_taus_label"        % gen_taus_label_;

    if (fs_) {
      hists_[0] = new Dynamic2DHistoSet(fs_->mkdir("httpairselector"));
      for (unsigned i = 0; i < hists_.size(); ++i) {
        hists_[i]->Create("n_pairs", 4, -0.5, 3.5, 4, -0.5, 3.5);
        hists_[i]->Create("pt_gen_reco", 50, 0, 100, 50, 0, 100);
      }
    }

    return 0;
  }

  int HTTPairSelector::Execute(TreeEvent *event) {

    // ************************************************************************
    // Do the actual pair selection, either by highest pT or most isolated tau
    // ************************************************************************
    std::vector<CompositeCandidate *> & dilepton = event->GetPtrVec<CompositeCandidate>(pair_label_);
    std::vector<CompositeCandidate *> os_dilepton;
    std::vector<CompositeCandidate *> ss_dilepton;
    std::vector<CompositeCandidate *> result;

    for (unsigned i = 0; i < dilepton.size(); ++i) {
      if (PairOppSign(dilepton[i])) os_dilepton.push_back(dilepton[i]);
      if (PairSameSign(dilepton[i])) ss_dilepton.push_back(dilepton[i]);
    }

    if (fs_) {
      EventInfo const* eventInfo = event->GetPtr<EventInfo>("eventInfo");
      double wt = eventInfo->total_weight();
      hists_[0]->Fill("n_pairs", os_dilepton.size(), ss_dilepton.size(), wt);
    }

    // The first pair should have the highest "scalar sum pt" (0,1 = tau_h, 2 = muon) pT
    std::sort(os_dilepton.begin(),os_dilepton.end(), boost::bind(&CompositeCandidate::ScalarPtSum, _1) > boost::bind(&CompositeCandidate::ScalarPtSum, _2));
    std::sort(ss_dilepton.begin(),ss_dilepton.end(), boost::bind(&CompositeCandidate::ScalarPtSum, _1) > boost::bind(&CompositeCandidate::ScalarPtSum, _2));
    
    // Or alternatively sort by isolation
    if (use_most_isolated_ && channel_ != channel::em) {
      std::sort(os_dilepton.begin(), os_dilepton.end(), [](CompositeCandidate const* c1, CompositeCandidate const* c2) {
        Tau const* tau1  = dynamic_cast<Tau const*>(c1->GetCandidate("lepton2"));
        Tau const* tau2  = dynamic_cast<Tau const*>(c2->GetCandidate("lepton2"));
        return (tau1->GetTauID("byCombinedIsolationDeltaBetaCorrRaw3Hits") < tau2->GetTauID("byCombinedIsolationDeltaBetaCorrRaw3Hits"));
      });
      std::sort(ss_dilepton.begin(), ss_dilepton.end(), [](CompositeCandidate const* c1, CompositeCandidate const* c2) {
        Tau const* tau1  = dynamic_cast<Tau const*>(c1->GetCandidate("lepton2"));
        Tau const* tau2  = dynamic_cast<Tau const*>(c2->GetCandidate("lepton2"));
        return (tau1->GetTauID("byCombinedIsolationDeltaBetaCorrRaw3Hits") < tau2->GetTauID("byCombinedIsolationDeltaBetaCorrRaw3Hits"));
      });
    }

    if (os_dilepton.size() > 0) { // Take OS in preference to SS
      result.push_back(os_dilepton[0]);
    } else if (ss_dilepton.size() > 0) {
      result.push_back(ss_dilepton[0]);
    }
    if (result.size() == 0) return 1;  //Require at least one dilepton

    // ************************************************************************
    // If using pair-wise mva met select the appropriate met
    // ************************************************************************
    if (mva_met_from_vector_) {
      std::map<std::size_t, Met *> const& met_map = event->GetIDMap<Met>("pfMVAMetVector");
      std::size_t id = 0;
      boost::hash_combine(id, result[0]->GetCandidate("lepton1")->id());
      boost::hash_combine(id, result[0]->GetCandidate("lepton2")->id());
      std::map<std::size_t, Met *>::const_iterator it = met_map.find(id);
      if (it != met_map.end()) {
        Met * mva_met = it->second;
        event->Add("pfMVAMet", mva_met);
      } else {
        std::cerr << "Could not find Met in collection for ID: " << id << std::endl;
        exit(0);
      }
    }
    
   // ************************************************************************
   // Scale met for the tau energy scale shift
   // ************************************************************************
    if (scale_met_for_tau_ && channel_ != channel::em) {
      Met * met = event->GetPtr<Met>(met_label_);
      Tau const* tau = dynamic_cast<Tau const*>(result[0]->GetCandidate("lepton2"));
      double t_scale = tau_scale_;
      if (event->Exists("tau_scales")) {
        std::map<std::size_t, double> const& tau_scales = event->Get< std::map<std::size_t, double>  > ("tau_scales");
        std::map<std::size_t, double>::const_iterator it = tau_scales.find(tau->id());
        if (it != tau_scales.end()) {
          t_scale = it->second;
        } else {
          std::cout << "Scale for chosen tau not found!" << std::endl;
          throw;
        }
      }
      double metx = met->vector().px();
      double mety = met->vector().py();
      double metet = met->vector().energy();
      double dx = tau->vector().px() * (( 1. / t_scale) - 1.);
      double dy = tau->vector().py() * (( 1. / t_scale) - 1.);
      metx = metx + dx;
      mety = mety + dy;
      metet = sqrt(metx*metx + mety*mety);
      ROOT::Math::PxPyPzEVector new_met(metx, mety, 0, metet);
      met->set_vector(ROOT::Math::PtEtaPhiEVector(new_met));
    }
    // ************************************************************************
    // Scale met for the electron energy scale shift
    // ************************************************************************
    if (scale_met_for_tau_ && channel_ == channel::em) {
      Met * met = event->GetPtr<Met>(met_label_);
      Electron const* elec = dynamic_cast<Electron const*>(result[0]->GetCandidate("lepton1"));
      double metx = met->vector().px();
      double mety = met->vector().py();
      double metet = met->vector().energy();
      double dx = elec->vector().px() * (( 1. / tau_scale_) - 1.);
      double dy = elec->vector().py() * (( 1. / tau_scale_) - 1.);
      metx = metx + dx;
      mety = mety + dy;
      metet = sqrt(metx*metx + mety*mety);
      ROOT::Math::PxPyPzEVector new_met(metx, mety, 0, metet);
      met->set_vector(ROOT::Math::PtEtaPhiEVector(new_met));
    }

    // ************************************************************************
    // Select l->tau or jet->tau fakes
    // ************************************************************************
    // mode 0 = e-tau, mode 1 = mu-tau, mode 2 = e-mu
    // faked_tau_selector = 1 -> ZL, = 2 -> ZJ
    // This code only to be run on Z->ee or Z->mumu events (remove Z->tautau first!)
    if (faked_tau_selector_ > 0  && channel_ != channel::em) {
      std::vector<GenParticle *> const& particles = event->GetPtrVec<GenParticle>("genParticles");
      std::vector<GenParticle *> sel_particles;
      if (channel_ == channel::et || channel_ == channel::etmet) {
        // Add all status 3 electrons with pT > 8 to sel_particles
        for (unsigned i = 0; i < particles.size(); ++i) {
          if ( (abs(particles[i]->pdgid()) == 11 || abs(particles[i]->pdgid()) == 13) && particles[i]->pt() > 8.) sel_particles.push_back(particles[i]);
        }
      } else if (channel_ == channel::mt || channel_ == channel::mtmet) {
        // Add all status 3 muons with pT > 8 to sel_particles
       for (unsigned i = 0; i < particles.size(); ++i) {
         if ( (abs(particles[i]->pdgid()) == 11 || abs(particles[i]->pdgid()) == 13) && particles[i]->pt() > 8.) sel_particles.push_back(particles[i]);
       } 
      }
      // Get the reco tau from the pair
      std::vector<Candidate *> tau;
      tau.push_back(result[0]->GetCandidate("lepton2"));
      // Get the matches vector - require match within DR = 0.5, and pick the closest gen particle to the tau
      std::vector<std::pair<Candidate*, GenParticle*> > matches = MatchByDR(tau, sel_particles, 0.5, true, true);
      // If we want ZL and there's no match, fail the event
      if (faked_tau_selector_ == 1 && matches.size() == 0) return 1;
      // If we want ZJ and there is a match, fail the event
      if (faked_tau_selector_ == 2 && matches.size() > 0) return 1;
    }
    if (hadronic_tau_selector_ > 0 && channel_ != channel::em) {
      std::vector<GenParticle *> const& particles = event->GetPtrVec<GenParticle>(gen_taus_label_);
      std::vector<GenJet> gen_taus = BuildTauJets(particles, false);
      std::vector<GenJet *> gen_taus_ptr;
      for (auto & x : gen_taus) gen_taus_ptr.push_back(&x);
      ic::erase_if(gen_taus_ptr, !boost::bind(MinPtMaxEta, _1, 18.0, 999.));
      std::vector<Candidate *> tau;
      tau.push_back(result[0]->GetCandidate("lepton2"));
      // Get the matches vector - require match within DR = 0.5, and pick the closest gen particle to the tau
      std::vector<std::pair<Candidate*, GenJet*> > matches = MatchByDR(tau, gen_taus_ptr, 0.5, true, true);
      // If we want ZL and there's no match, fail the event
      if (hadronic_tau_selector_ == 1 && matches.size() == 0) return 1;
      if (fs_ && matches.size() == 1) {
        hists_[0]->Fill("pt_gen_reco", matches[0].second->pt(), matches[0].first->pt(), 1);
      }
      // If we want ZJ and there is a match, fail the event
      if (hadronic_tau_selector_ == 2 && matches.size() > 0) return 1;
    }
    // ************************************************************************
    // Restrict decay modes
    // ************************************************************************
    Tau const* tau_ptr = dynamic_cast<Tau const*>(result[0]->GetCandidate("lepton2"));
    if (tau_ptr && allowed_tau_modes_ != "") {
      if (tau_mode_set_.find(tau_ptr->decay_mode()) == tau_mode_set_.end()) return 1;
    }

    dilepton = result;
    return 0;
  }
  int HTTPairSelector::PostAnalysis() {
    return 0;
  }

  void HTTPairSelector::PrintInfo() {
    ;
  }
}
