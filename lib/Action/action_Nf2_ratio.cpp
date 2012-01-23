/*!
 * @file action_Nf2_ratio.cpp
 *
 * @brief Definition of methods of Action_Nf2_ratio class
 *
 */

#include "action_Nf2_ratio.hpp"

Field Action_Nf2_ratio::DdagD1_inv(const Field& src){
  Field sol(fsize_);
  SolverOutput monitor = slv1_->solve(sol,src);
#if VERBOSITY >= SOLV_MONITOR_VERB_LEVEL
  monitor.print();
#endif
  return sol;
}

Field Action_Nf2_ratio::DdagD2_inv(const Field& src){
  Field sol(fsize_);
  SolverOutput monitor = slv2_->solve(sol,src);
#if VERBOSITY >= SOLV_MONITOR_VERB_LEVEL
  monitor.print();
#endif
  return sol;
}

void Action_Nf2_ratio::init(const RandNum& rand,const void*){
  std::valarray<double> ph(fsize_);
  
  MPrand::mp_get_gauss(ph,rand,D1_->get_gsite(),D1_->get_fermionFormat());

  #if VERBOSITY>=DEBUG_VERB_LEVEL
  double phsum= (ph*ph).sum();
  double phnorm= Communicator::instance()->reduce_sum(phsum);
  CCIO::cout<<"fsize_="<<fsize_<<std::endl;
  CCIO::cout<<"ph.norm="<<sqrt(phnorm)<<std::endl;
  #endif 
  phi_= D1_->mult_dag(Field(ph));

  #if VERBOSITY>=DEBUG_VERB_LEVEL
  double phisum= phi_.norm();
  double phinorm= Communicator::instance()->reduce_sum(phisum);
  #endif
  phi_= D2_->mult(DdagD2_inv(phi_));
}

double Action_Nf2_ratio::calc_H(){
  Field zeta = D2_->mult_dag(phi_);//2 flavors
  double H_Ratio = zeta * DdagD1_inv(zeta);
  _Message(ACTION_VERB_LEVEL, "    [Action_Nf2_ratio] H = " << H_Ratio <<"\n");
  return H_Ratio;
}
  
Field Action_Nf2_ratio::md_force(const void*){
  Field eta = DdagD1_inv(D2_->mult_dag(phi_));
  Field force= D1_->md_force(eta,D1_->mult(eta));
  force -= D2_->md_force(eta,phi_);
  _MonitorMsg(ACTION_VERB_LEVEL, Action, force, "Action_Nf2_ratio");
  return force;
}

Action_Nf2_ratio::~Action_Nf2_ratio(){}
