/*
 * @file mdExec_leapfrog.hpp
 * @brief Declarations of MDexec_leapfrog class and Parameters 
 */
#ifndef MD_LEAPFROG_INCLUDED
#define MD_LEAPFROG_INCLUDED

#include "mdExec.hpp"
#include "Action/action.hpp"
#include "include/pugi_interface.h"
#include "Smearing/smartConf.hpp"
#include<vector>

struct MDexec_leapfrogParams{
  int Nexp;
  int MDsteps;
  double step_size;

  MDexec_leapfrogParams(XML::node node){
    XML::read(node, "MDsteps", MDsteps);
    XML::read(node, "step_size" , step_size);
    XML::read(node, "exp_approx", Nexp);
  }

  MDexec_leapfrogParams(int Nexp_,int MDsteps_,double step)
    :Nexp(Nexp_),MDsteps(MDsteps_),step_size(step){}
};

class MDexec_leapfrog : public MDexec {
private:
  const MDexec_leapfrogParams Params;
  const std::vector<int> Nrel_;
  const ActionSet as_;
  ObserverList observers_;
  GaugeField* const U_;
  GaugeField P_;

  // Private functions
  void update_P(int lv,double ep);
  void update_U(double ep);
  void integrator_step(int level,std::vector<int>& clock);

  // Observers controls
  void register_observers();
  void notify_observers(); 

public:
  MDexec_leapfrog(int Nexp, int MDiter, double step,
		  const ActionSet as,
		  const std::vector<int> multipliers,
		  SmartConf* const CommonU)
    :as_(as),
     Params(MDexec_leapfrogParams(Nexp,MDiter,step)),
     Nrel_(multipliers),
     U_(CommonU->ThinLinks){
    observers_.push_back(CommonU);//Attach smearing as 1st observer
    register_observers();
  }
  
  MDexec_leapfrog(int Nexp, int MDiter, double step,
   		  const ActionSet as,
   		  const std::vector<int> multipliers,
   		  GaugeField* const CommonU)
    :as_(as),
     Params(MDexec_leapfrogParams(Nexp,MDiter,step)),
     Nrel_(multipliers),
     U_(CommonU),P_(){
    register_observers();
  }
  
  MDexec_leapfrog(XML::node node,
		  const ActionSet as,
		  const std::vector<int> multipliers,
		  GaugeField* const CommonU)
    :as_(as),
     Params(MDexec_leapfrogParams(node)),
     Nrel_(multipliers),
     U_(CommonU),P_(){
    register_observers();
  }
  
  MDexec_leapfrog(XML::node node,
		  const ActionSet as,
		  const std::vector<int> multipliers,
		  SmartConf* const CommonU)
    :as_(as),
     Params(MDexec_leapfrogParams(node)),
     Nrel_(multipliers),
     U_(CommonU->ThinLinks){
    observers_.push_back(CommonU);//Attach smearing as 1st observer
    register_observers();
  }


  void init(std::vector<int>& clock,const GaugeField& U,const RandNum& rand);
  void integrator(int level,std::vector<int>& clock);
  double calc_H() const;
  const GaugeField get_U() const;
};

#endif  //MD_LEAPFROG_INCLUDED
