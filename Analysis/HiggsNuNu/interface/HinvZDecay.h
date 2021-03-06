#ifndef ICHiggsTauTau_Module_HinvZDecay_h
#define ICHiggsTauTau_Module_HinvZDecay_h

#include "UserCode/ICHiggsTauTau/Analysis/Core/interface/TreeEvent.h"
#include "UserCode/ICHiggsTauTau/Analysis/Core/interface/ModuleBase.h"
#include <string>


namespace ic {

class HinvZDecay : public ModuleBase {
 private:

    unsigned flavour_;

    unsigned countStatus3_;
    double minMll_;
    double maxMll_;

 public:
    HinvZDecay(std::string const& name, unsigned flavour,
	       double minMll=60., double maxMll=120.);
    virtual ~HinvZDecay();
    
    virtual int PreAnalysis();
    virtual int Execute(TreeEvent *event);
    virtual int PostAnalysis();
    virtual void PrintInfo();
    
};

}

#endif
