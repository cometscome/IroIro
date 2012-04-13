/*! 
 * @file action_Nf2.cpp
 * @brief Definition of methods of Action_Nf2 class
 */
#include "action_Nf2.hpp"
#include "Tools/randNum_MP.h"
#include "Tools/fieldUtils.hpp"
#include "include/messages_macros.hpp"

//::::::::::::::::::::::::::::::::Observer
void Action_Nf2::observer_update() {
  D_->update_internal_state();  
}

void Action_Nf2::attach_smearing(SmartConf* SmearObj) {
  // Checks that the pointer for gauge field u_ 
  // points correctly to the smeared configuration
  // otherwise falls back to standard update
  if (u_ == SmearObj->get_current_conf()) {
    smart_conf_= SmearObj; //set the configuration
    CCIO::cout << "Succesfully attached smearing routines\n";
  }else{
    CCIO::cout << "Pointers disagree - Smearing not allowed\n";
    smeared_ = false;
  }
}


Field Action_Nf2::DdagD_inv(const Field& src){
  Field sol(fsize_);
  SolverOutput monitor = slv_->solve(sol,src);
#if VERBOSITY >= SOLV_MONITOR_VERB_LEVEL
  monitor.print();
#endif
  return sol;
}

void Action_Nf2::init(const RandNum& rand){
  CCIO::cout << "Action Initialization\n";
  std::valarray<double> ph(fsize_);
  MPrand::mp_get_gauss(ph,rand,D_->get_gsite(),D_->get_fermionFormat());
  phi_= D_->mult_dag(Field(ph));
  CCIO::cout << "Action Initialization phi.norm "<< phi_.norm() << "\n";
}

double Action_Nf2::calc_H(){ 
  CCIO::cout << "Action calculation\n";
  double H_nf2 = phi_*DdagD_inv(phi_);
  _Message(ACTION_VERB_LEVEL, "    [Action_Nf2] H = "<< H_nf2<<"\n");
  return H_nf2;
}

GaugeField Action_Nf2::md_force(){
  Field eta = DdagD_inv(phi_);
  GaugeField fce(D_->md_force(eta,D_->mult(eta)));
  
  // [fce] is [U*SigmaTilde] in smearing language
  if(smart_conf_) smart_conf_->smeared_force(fce);
  GaugeField force = FieldUtils::TracelessAntihermite(fce);

  _MonitorMsg(ACTION_VERB_LEVEL, Action, force, "Action_Nf2");
  return force;
}
