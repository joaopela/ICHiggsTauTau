#ifndef ICHiggsTauTau_Module_HTTCategories_h
#define ICHiggsTauTau_Module_HTTCategories_h

#include "UserCode/ICHiggsTauTau/Analysis/Core/interface/TreeEvent.h"
#include "UserCode/ICHiggsTauTau/Analysis/Core/interface/ModuleBase.h"
#include "PhysicsTools/FWLite/interface/TFileService.h"
#include "UserCode/ICHiggsTauTau/Analysis/Utilities/interface/HTTPlots.h"
#include "UserCode/ICHiggsTauTau/Analysis/HiggsTauTau/interface/HTTConfig.h"

#include <string>

namespace ic {

class HTTCategories : public ModuleBase {

 private:
  std::string jets_label_;
  CLASS_MEMBER(HTTCategories, std::string, ditau_label)
  CLASS_MEMBER(HTTCategories, std::string, met_label)
  // CLASS_MEMBER(HTTCategories, double, mt_max_selection)
  // CLASS_MEMBER(HTTCategories, double, mt_min_control)
  // CLASS_MEMBER(HTTCategories, double, mt_max_control)
  // CLASS_MEMBER(HTTCategories, double, pzeta_min_selection)
  // CLASS_MEMBER(HTTCategories, double, pzeta_max_control)
  // CLASS_MEMBER(HTTCategories, double, pzeta_alpha)
  CLASS_MEMBER(HTTCategories, ic::channel, channel)
  // CLASS_MEMBER(HTTCategories, bool, distinguish_os)
  CLASS_MEMBER(HTTCategories, fwlite::TFileService*, fs)

  std::map<std::string, bool> categories_;
  std::map<std::string, bool> selections_;
  std::map<std::string, MassPlots*> massplots_;
  std::map<std::string, double> yields_;
  std::map<std::string, CoreControlPlots*> controlplots_;


  // Event Properties
    bool os_;

    double wt_;

    unsigned n_vtx_;
    double m_sv_;
    double m_vis_;
    double mt_1_;
    double pzeta_;
    double pt_1_;
    double pt_2_;
    double eta_1_;
    double eta_2_;
    double met_;
    double met_phi_;

    unsigned n_jets_;
    unsigned n_lowpt_jets_;
    unsigned n_bjets_;
    unsigned n_jetsingap_; // Defined if n_jets >= 2
    double jpt_1_;     // Defined if n_jets >= 1
    double jpt_2_;     // Defined if n_jets >= 2
    double jeta_1_;    // Defined if n_jets >= 1
    double jeta_2_;    // Defined if n_jets >= 2
    double bpt_1_;     // Defined if n_bjets >= 1
    double beta_1_;    // Defined if n_bjets >= 1

    double mjj_;       // Defined if n_jets >= 2
    double jdeta_;     // Defined if n_jets >= 2

    // Other VBF MVA variables?
  


 public:
  HTTCategories(std::string const& name);
  virtual ~HTTCategories();

  virtual int PreAnalysis();
  virtual int Execute(TreeEvent *event);
  virtual int PostAnalysis();
  virtual void PrintInfo();

  bool PassesCategory(std::string const& category) const;

  void InitSelection(std::string const& selection);
  void InitCategory(std::string const& category);
  void InitMassPlots(std::string const& category);
  void FillMassPlots(std::string const& category);
  void FillYields(std::string const& category);
  void InitCoreControlPlots(std::string const& category);
  void FillCoreControlPlots(std::string const& category);

  void SetPassCategory(std::string const& category);
  void SetPassSelection(std::string const& selection);

  void Reset();


};

}


#endif