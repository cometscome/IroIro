/*!--------------------------------------------------------------------------
 * @file dirac_DomainWall.cpp
 * @brief Definition of class methods for Dirac_optimalDomainWall (5d operator)
 Time-stamp: <2013-05-21 09:25:31 noaki>
 *-------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <math.h>
#include <gsl/gsl_sf_ellint.h>
#include <gsl/gsl_sf_elljac.h>

#include "dirac_DomainWall.hpp"
#include "Communicator/comm_io.hpp"
#include "Fields/field_expressions.hpp"

#ifdef IBM_BGQ_WILSON
#include "bgqwilson.h"
#include <omp.h>
#include <hwi/include/bqc/A2_inlines.h>
#endif

using namespace std;

// Constructors for DomainWall Parameters classes
//======================================================================
Dirac_optimalDomainWall_params::
Dirac_optimalDomainWall_params(XML::node DWF_node,DWFType Type){
  /* temporal hack*/
  XML::node mynode = DWF_node;
  XML::descend(mynode,"BaseKernel", MANDATORY);
  XML::read(mynode, "mass", M0_, MANDATORY);

  std::string Precond_string;
  XML::read(DWF_node, "Preconditioning", Precond_string, MANDATORY);
  XML::read(DWF_node, "N5d", N5_, MANDATORY);
  XML::read(DWF_node, "b", b_, MANDATORY);
  XML::read(DWF_node, "c", c_, MANDATORY);
  XML::read(DWF_node, "mass", mq_, MANDATORY);

  if(Type == PauliVillars) mq_= 1.0;
  
  XML::node ApproxNode = DWF_node.child("approximation");
  if(ApproxNode !=NULL) {
    const char* Approx_name = ApproxNode.attribute("name").value();
    if(!strcmp(Approx_name, "Zolotarev")){
      double lambda_min, lambda_max;
      XML::read(ApproxNode, "lambda_min", lambda_min); 
      XML::read(ApproxNode, "lambda_max", lambda_max); 
      omega_= DomainWallFermions::getOmega(N5_,lambda_min,lambda_max);
    }
    if (!strcmp(Approx_name, "Tanh"))  
      for (int s=0; s<N5_; ++s) omega_.push_back(1.0);
  }else{
    CCIO::cout << "Error: missing [approximation] node or wrong entry\n";
    abort();
  }
  // setup of the member arrays
  set_arrays();
  if (!EnumString<Preconditioners>::To( Preconditioning_, Precond_string )){
    CCIO::cerr << "Error: string ["<< Precond_string <<"] not valid" 
	       <<std::endl;
    abort();
  } else {
    CCIO::cout << "Choosing preconditioner type: "
	       << Precond_string << " Code: "<< Preconditioning_ <<std::endl;
  }
}

Dirac_optimalDomainWall_params::
Dirac_optimalDomainWall_params(double b,double c,double M0,double mq,
			       const std::vector<double>& omega, 
			       Preconditioners Preconditioning)
  :N5_(omega.size()),b_(b),c_(c),M0_(M0),mq_(mq),omega_(omega),
   Preconditioning_(Preconditioning){
  set_arrays();
}

void Dirac_optimalDomainWall_params::set_arrays(){

  for(int s=0; s<N5_; ++s){
    bs_.push_back(0.5*(b_*omega_[s] +c_));
    cs_.push_back(0.5*(b_*omega_[s] -c_));
    dp_.push_back(bs_[s]*(4.0 +M0_)+1.0);
    dm_.push_back(1.0 -cs_[s]*(4.0 +M0_));
  }
  es_.push_back(dm_[N5_-1]/dp_[0]);
  fs_.push_back(dm_[0]);
  for (int s=1; s<N5_-1; ++s) {
    es_.push_back(es_[s-1]*(dm_[s-1]/dp_[s]));
    fs_.push_back(fs_[s-1]*(dm_[s]/dp_[s-1]));
  }
}

///////////////////////////////////////////////////////////////////////////
/*! @brief mult without 4D-hopping parameters */

const Field Dirac_optimalDomainWall::mult_hop5(const Field& f5) const{
  Field w5(f5size_), v(f4size_);
  
#ifdef IBM_BGQ_WILSON
  double* v_ptr = v.getaddr(0);

  for(int s=0; s<N5_; ++s) {
    double* w5_ptr = w5.getaddr(ff_.index(0,0,s));
    double* f5_ptr = const_cast<Field&>(f5).getaddr(ff_.index(0,0,s));
    BGWilsonLA_MultScalar(w5_ptr,f5_ptr,Params_.dp_[s],Nvol_);
  }
  for(int s=0; s<N5_-1; ++s) {
    double* w5_ptr = w5.getaddr(ff_.index(0,0,s));
    BGWilsonLA_Proj_M(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,s+1)),Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,-Params_.dm_[s],Nvol_);
  }
  for(int s=1; s<N5_; ++s) {
    double* w5_ptr = w5.getaddr(ff_.index(0,0,s));
    BGWilsonLA_Proj_P(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,s-1)),Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,-Params_.dm_[s],Nvol_);
  }
  double* w5_ptr = w5.getaddr(ff_.index(0,0,0));
  BGWilsonLA_Proj_P(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,N5_-1)),Nvol_);
  BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,mq_*Params_.dm_[0],Nvol_);

  w5_ptr = w5.getaddr(ff_.index(0,0,N5_-1));
  BGWilsonLA_Proj_M(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,0)),Nvol_);
  BGWilsonLA_MultAddScalar(w5_ptr, v_ptr,mq_*Params_.dm_[N5_-1],Nvol_);
#else
  using namespace FieldExpression;

  for(int s=0; s<N5_; ++s){
    Field v = get4d(f5,s);
    v *= Params_.dp_[s];
    set5d(w5,v,s);
  }
  for(int s=0; s<N5_-1; ++s){
    proj_m(v,f5,s+1);
    v *= -Params_.dm_[s];
    add5d(w5,v,s);
  }
  for(int s=1; s<N5_; ++s){
    proj_p(v,f5,s-1);
    v *= -Params_.dm_[s];
    add5d(w5,v,s);
  }

  proj_p(v,f5,N5_-1);
  v *= mq_*Params_.dm_[0];
  add5d(w5,v,0);

  proj_m(v,f5,0);
  v *= mq_*Params_.dm_[N5_-1];
  add5d(w5,v,N5_-1);
#endif

  return w5;
}

const Field Dirac_optimalDomainWall::mult_hop5_dag(const Field& f5) const{
  Field w5(f5size_), v(f4size_);

#ifdef IBM_BGQ_WILSON
  double* v_ptr = v.getaddr(0);

  for(int s=0; s<N5_; ++s){
    double* w5_ptr = w5.getaddr(ff_.index(0,0,s));
    double* f5_ptr = const_cast<Field&>(f5).getaddr(ff_.index(0,0,s));
    BGWilsonLA_MultScalar(w5_ptr,f5_ptr,Params_.dp_[s],Nvol_);
  }
  for(int s=0; s<N5_-1; ++s){
    double* w5_ptr = w5.getaddr(ff_.index(0,0,s));
    BGWilsonLA_Proj_P(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,s+1)),Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,-Params_.dm_[s+1],Nvol_);
  }
  for(int s=1; s<N5_; ++s){
    double* w5_ptr = w5.getaddr(ff_.index(0,0,s));
    BGWilsonLA_Proj_M(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,s-1)),Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,-Params_.dm_[s-1],Nvol_);
  }
  double* w5_ptr = w5.getaddr(ff_.index(0,0,0));
  BGWilsonLA_Proj_M(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,N5_-1)),Nvol_);
  BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,mq_*Params_.dm_[N5_-1],Nvol_);

  w5_ptr = w5.getaddr(ff_.index(0,0,N5_-1));
  BGWilsonLA_Proj_P(v_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,0)),Nvol_);
  BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,mq_*Params_.dm_[0],Nvol_);

#else
  assert(f5.size()==f5size_);
  for(int s=0; s<N5_; ++s){
    v = get4d(f5,s);
    v *= Params_.dp_[s];
    set5d(w5,v,s);
  }
  for(int s=0; s<N5_-1; ++s){
    proj_p(v,f5,s+1);
    v *= -Params_.dm_[s+1];
    add5d(w5,v,s);
  }
  for(int s=1; s<N5_; ++s){
    proj_m(v,f5,s-1);
    v *= -Params_.dm_[s-1];
    add5d(w5,v,s);
  }
  proj_m(v,f5,N5_-1);
  v *= mq_*Params_.dm_[N5_-1];
  add5d(w5,v,0);

  proj_p(v,f5,0);
  v *= mq_*Params_.dm_[0];
  add5d(w5,v,N5_-1);
#endif

  return w5;
}

const Field Dirac_optimalDomainWall::mult_hop5_inv(const Field& f5) const{
#ifdef IBM_BGQ_WILSON

  Field w5(f5),lpf(f4size_),ey(f4size_),lmf(f4size_),fy(f4size_);
  double* lpf_ptr = lpf.getaddr(0);
  double* ey_ptr = ey.getaddr(0);
  double* fy_ptr = fy.getaddr(0);
  double* lmf_ptr = lmf.getaddr(0);

  double* w5_ptr_bdry   = w5.getaddr((N5_-1)*f4size_);
  BGWilsonLA_Proj_M(ey_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,0)),Nvol_);
  BGWilsonLA_MultAddScalar(w5_ptr_bdry,     ey_ptr,-mq_* Params_.es_[0],Nvol_);

  for (int s=1; s<N5_-1; ++s) {
    double* w5_ptr   = w5.getaddr(s*f4size_);
    double fact_lpf = (Params_.dm_[s]/Params_.dp_[s-1]);
    double fact_ey =  mq_*Params_.es_[s];

    BGWilsonLA_Proj_P(lpf_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,s-1)),Nvol_);
    BGWilsonLA_Proj_M(ey_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,s)),Nvol_);
 
    BGWilsonLA_MultAddScalar(w5_ptr,     lpf_ptr,fact_lpf,Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr_bdry,ey_ptr,-fact_ey,Nvol_);

  }
  BGWilsonLA_Proj_P(lpf_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,N5_-2)),Nvol_);
  BGWilsonLA_MultAddScalar(w5_ptr_bdry,lpf_ptr,(Params_.dm_[N5_-1]/Params_.dp_[N5_-2]),Nvol_);

  double fact= 1.0/(Params_.dp_[N5_-1] +mq_*Params_.dm_[N5_-2]*Params_.es_[N5_-2]);
  BGWilsonLA_MultScalar(w5_ptr_bdry, w5_ptr_bdry, fact, Nvol_);


  for(int s=N5_-2; s>=0; --s) {
    double* w5_ptr   = w5.getaddr(s*f4size_);
    BGWilsonLA_Proj_M(lmf_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,s+1)),Nvol_);
    BGWilsonLA_Proj_P(fy_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,N5_-1)),Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,     lmf_ptr,Params_.dm_[s],Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,     fy_ptr,-mq_*Params_.fs_[s],Nvol_);
    BGWilsonLA_MultScalar(w5_ptr, w5_ptr, 1.0/ Params_.dp_[s], Nvol_);
  }
#else
  Field w5(f5);
  Field lf(f4size_),fy(f4size_);

  proj_m(fy,w5,0);
  fy *= -mq_* Params_.es_[0];
  add5d(w5,fy,N5_-1);

  for(int s=1; s<N5_-1; ++s){
    proj_p(lf,w5,s-1);
    proj_m(fy,w5,s);

    lf *= Params_.dm_[s]/Params_.dp_[s-1];
    add5d(w5,lf,s);

    fy *= -mq_*Params_.es_[s];
    add5d(w5,fy,N5_-1);
  }
  proj_p(lf,w5,N5_-2);
  lf *= (Params_.dm_[N5_-1]/Params_.dp_[N5_-2]);
  add5d(w5,lf,N5_-1);

  mul5d(w5,1.0/(Params_.dp_[N5_-1] +mq_*Params_.dm_[N5_-2]*Params_.es_[N5_-2]),N5_-1);  

  for(int s=N5_-2; s>=0; --s){
    proj_m(lf,w5,s+1);
    lf *= Params_.dm_[s];
    add5d(w5,lf,s);
    proj_p(fy,w5,N5_-1);
    fy *= -mq_*Params_.fs_[s];
    add5d(w5,fy,s);
    Field v = get4d(w5,s);
    v*= 1.0/ Params_.dp_[s];
    set5d(w5,v,s);
  }
#endif
  return w5;
}

const Field Dirac_optimalDomainWall::mult_hop5_dinv(const Field& f5) const{
#ifdef IBM_BGQ_WILSON
  
  Field w5(f5),lpf(f4size_),ey(f4size_),lmf(f4size_),v(f4size_);
  double* w5_ptr  = w5.getaddr(0);
  double* v_ptr   = v.getaddr(0);
  double* lpf_ptr = lpf.getaddr(0);
  double* lmf_ptr = lmf.getaddr(0);
  double* ey_ptr  = ey.getaddr(0);

  BGWilsonLA_MultScalar(w5_ptr,w5_ptr,1.0/Params_.dp_[0],Nvol_);

  for(int s=1; s<N5_-1; ++s){
    w5_ptr = w5.getaddr(s*f4size_);
    BGWilsonLA_Proj_M(lmf_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,s-1)),Nvol_);
    BGWilsonLA_MultAddScalar(w5_ptr,lmf_ptr,Params_.dm_[s-1],Nvol_);
    BGWilsonLA_MultScalar(w5_ptr,w5_ptr,1.0/Params_.dp_[s], Nvol_);   
  }
  w5_ptr   = w5.getaddr((N5_-1)*f4size_); 
  BGWilsonLA_Proj_M(v_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,N5_-2)),Nvol_);   
  BGWilsonLA_MultAddScalar(w5_ptr,v_ptr,Params_.dm_[N5_-2],Nvol_);
  for(int s=0; s<N5_-1; ++s) {
    BGWilsonLA_Proj_P(ey_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,s)),Nvol_); 
    BGWilsonLA_MultAddScalar(w5_ptr,ey_ptr,-mq_*Params_.fs_[s],Nvol_);
  }
  BGWilsonLA_MultScalar(w5_ptr,w5_ptr,1.0/(Params_.dp_[N5_-1] +mq_*Params_.dm_[N5_-2]*Params_.es_[N5_-2]),Nvol_);
  
  for(int s=N5_-2; s>=0; --s){ 
    double* w5_ptr   = w5.getaddr(s*f4size_);
    double fact_lpf = (Params_.dm_[s+1]/Params_.dp_[s]);

    BGWilsonLA_Proj_P(lpf_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,s+1)),Nvol_); 
    BGWilsonLA_Proj_M(ey_ptr,const_cast<Field&>(w5).getaddr(ff_.index(0,0,N5_-1)),Nvol_);
    BGWilsonLA_AXPBYPZ(w5_ptr,lpf_ptr,ey_ptr,w5_ptr,fact_lpf,-mq_*Params_.es_[s],Nvol_);
  }
#else
  Field w5(f5),lpf(f4size_),ey(f4size_),lmf(f4size_),fy(f4size_),v(f4size_);
  double* lpf_ptr = lpf.getaddr(0);
  double* ey_ptr = ey.getaddr(0);

  v = get4d(w5,0);
  v *= 1.0/Params_.dp_[0];
  set5d(w5,v,0);

  for(int s=1; s<N5_-1; ++s){
    proj_m(lmf,w5,s-1);                                         
    lmf *= Params_.dm_[s-1];                                                     
    add5d(w5,lmf,s);                                                            
    v = get4d(w5,s);                                                            
    v*= 1.0/Params_.dp_[s];                                                      
    set5d(w5,v,s);                                                              
  }                                                                             
  proj_m(v,w5,N5_-2);                                                  
  v*= Params_.dm_[N5_-2];                                                        
  add5d(w5,v,N5_-1);                                                            
  for(int s=0; s<N5_-1; ++s) {                                                  
    proj_p(fy,w5,s);                                             
    fy *= -mq_*Params_.fs_[s];                                            
    add5d(w5,fy,N5_-1);                                                         
  }                                                                             
  v = get4d(w5,N5_-1);                                                          
  v*= 1.0/(Params_.dp_[N5_-1] +mq_*Params_.dm_[N5_-2]*Params_.es_[N5_-2]);    
  set5d(w5,v,N5_-1);                                                            
                                                                                
  for(int s=N5_-2; s>=0; --s){ 
    double* w5_ptr   = w5.getaddr(s*f4size_);
    double fact_lpf = (Params_.dm_[s+1]/Params_.dp_[s]);
    double fact_ey =  mq_*Params_.es_[s];

    proj_p(lpf,w5,  s+1);                                          
    proj_m( ey,w5,N5_-1); 

    for (int i=0; i<f4size_; ++i) {
      w5_ptr[i] += fact_lpf*lpf_ptr[i]- fact_ey*ey_ptr[i];
    }                                                       
  }
#endif
  return w5;    
}

/*! @brief definitions of D_dwf */
void Dirac_optimalDomainWall::mult_full(Field& w5, const Field& f5) const{ 

  Field v(f4size_),lpf(f4size_),lmf(f4size_);
#pragma disjoint

#ifdef IBM_BGQ_WILSON
  double* v_ptr   = v.getaddr(0);
  double* lpf_ptr = lpf.getaddr(0);
  double* lmf_ptr = lmf.getaddr(0);

  Field w(f4size_);
  double* w_ptr = w.getaddr(0);

  for(int s=0; s<N5_; ++s) {
    double* f5_ptr = const_cast<Field&>(f5).getaddr(s*f4size_);
    double* w5_ptr   = w5.getaddr(s*f4size_);

    BGWilsonLA_Proj_P(lpf_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,(s+N5_-1)%N5_)),Nvol_);
    BGWilsonLA_Proj_M(lmf_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,(s+1)%N5_)),Nvol_);
    
    if(s==0)          BGWilsonLA_MultScalar_Add(lpf_ptr,lmf_ptr,-mq_,Nvol_);
    else if(s==N5_-1) BGWilsonLA_MultAddScalar(lpf_ptr,lmf_ptr,-mq_,Nvol_);
    else              BGWilsonLA_Add(lpf_ptr,lmf_ptr,Nvol_);

    BGWilsonLA_AXPBY(v_ptr,f5_ptr,lpf_ptr,Params_.bs_[s],Params_.cs_[s],Nvol_);
    
    Dw_->mult_ptr(w_ptr, v_ptr);  
    BGWilsonLA_AXPBYPZ(w5_ptr,w_ptr,lpf_ptr,f5_ptr,4.0+M0_,-1.0,Nvol_);
  }
#else
  using namespace FieldExpression;
  double* v_ptr   = v.getaddr(0);
  double* lpf_ptr = lpf.getaddr(0);
  double* lmf_ptr = lmf.getaddr(0);
  double mass_fact= 4.0+M0_;

  for(int s=0; s<N5_; ++s){
    double* f5_ptr = const_cast<Field&>(f5).getaddr(s*f4size_);


    proj_p(lpf,f5,(s+N5_-1)%N5_);
    if(s==0)     lpf *= -mq_;
    proj_m(lmf,f5,(s+1)%N5_);
    if(s==N5_-1) lmf *= -mq_;

    for(int i=0; i<f4size_; ++i){
      lpf_ptr[i] += lmf_ptr[i];
      v_ptr[i] = Params_.bs_[s]*f5_ptr[i]+Params_.cs_[s]*lpf_ptr[i];
    }
    Field w = Dw_->mult(v);
    double* w_ptr = w.getaddr(0);

    double* w5_ptr = w5.getaddr(s*f4size_);
    for(int i=0; i<f4size_; ++i)
      w5_ptr[i] = mass_fact*w_ptr[i]+ f5_ptr[i] -lpf_ptr[i];
  }
#endif
}

void Dirac_optimalDomainWall::mult_dag_full(Field& w5,const Field& f5) const{

  Field v5(f5size_);
  Field lpf(f4size_),lmf(f4size_);
  int spin_idx;
  double cs, bs;

#pragma disjoint
#ifdef IBM_BGQ_WILSON
  Field w(f4size_);
  double* w_ptr = w.getaddr(0);
 
  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    double* f5_ptr = const_cast<Field&>(f5).getaddr(spin_idx);
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);
    
    bs = (4.0+M0_)*Params_.bs_[s];
    cs = (4.0+M0_)*Params_.cs_[s];

    Dw_->mult_dag_ptr(w_ptr,f5_ptr);
    BGWilsonLA_AXPY(w5_ptr,w_ptr,f5_ptr,bs,Nvol_);
    BGWilsonLA_AXMY(v5_ptr,w_ptr,f5_ptr,cs,Nvol_);
  }
  
  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);
    
    BGWilsonLA_Proj_P(lpf.getaddr(0),
		      const_cast<Field&>(v5).getaddr(ff_.index(0,0,(s+1    )%N5_)),Nvol_);
    BGWilsonLA_Proj_M(lmf.getaddr(0),
		      const_cast<Field&>(v5).getaddr(ff_.index(0,0,(s+N5_-1)%N5_)),Nvol_);
    
    if(s==N5_-1)  BGWilsonLA_AXPBYPZ(w5_ptr,lpf.getaddr(0),lmf.getaddr(0),w5_ptr,-mq_, 1.0,Nvol_);
    else if(s==0) BGWilsonLA_AXPBYPZ(w5_ptr,lpf.getaddr(0),lmf.getaddr(0),w5_ptr, 1.0,-mq_,Nvol_);
    else          BGWilsonLA_AXPBYPZ(w5_ptr,lpf.getaddr(0),lmf.getaddr(0),w5_ptr, 1.0, 1.0,Nvol_);
  }
#else
  assert(w5.size()==f5.size());
  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    double* f5_ptr = const_cast<Field&>(f5).getaddr(spin_idx);
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);
    
    bs = (4.0+M0_)*Params_.bs_[s];
    cs = (4.0+M0_)*Params_.cs_[s];
    
    Field w = Dw_->mult_dag(get4d(f5,s));
    double* w_ptr = w.getaddr(0);
    
    for(int i=0; i<f4size_; ++i){
      w5_ptr[i] = bs*w_ptr[i];
      v5_ptr[i] = cs*w_ptr[i];
    }
    // do not change this
    for(int i=0; i<f4size_; ++i){
      w5_ptr[i] += f5_ptr[i];
      v5_ptr[i] -= f5_ptr[i];
    }
  }
  for(int s=0; s<N5_; ++s){
    proj_p(lpf,v5,(s+1)%N5_);
    if(s == N5_-1) lpf *= -mq_;
    proj_m(lmf,v5,(s+N5_-1)%N5_);
    if(s == 0)     lmf *= -mq_;
    add5d(w5,lpf,lmf,s);
  }
#endif
}

void Dirac_optimalDomainWall::mult_offdiag(Field& w5,const Field& f5) const{ 
#pragma disjoint
  #ifdef IBM_BGQ_WILSON
  Field lpf(f4size_), lmf(f4size_), w(f4size_), v(f4size_);
  double* w_ptr = w.getaddr(0);
  double* v_ptr = v.getaddr(0);
  double* lpf_ptr = lpf.getaddr(0);
  double* lmf_ptr = lmf.getaddr(0);
  double mass_fact= 4.0+M0_;
  double* f5_ptr = const_cast<Field&>(f5).getaddr(0);
  double* w5_ptr   = w5.getaddr(0);
  
  /* here, Nvol_ is half-size */
  for(int s=0; s<N5_; ++s) {
    BGWilsonLA_Proj_P(lpf_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,(s+N5_-1)%N5_)),Nvol_);
    BGWilsonLA_Proj_M(lmf_ptr,const_cast<Field&>(f5).getaddr(ff_.index(0,0,(s+1)%N5_)),Nvol_);

    if(     s==0)     BGWilsonLA_MultScalar_Add(lpf_ptr,lmf_ptr,-mq_,Nvol_);
    else if(s==N5_-1) BGWilsonLA_MultAddScalar(lpf_ptr,lmf_ptr,-mq_,Nvol_);
    else              BGWilsonLA_Add(lpf_ptr,lmf_ptr,Nvol_);

    BGWilsonLA_AXPBY(v_ptr,f5_ptr,lpf_ptr,Params_.bs_[s],Params_.cs_[s],Nvol_);
    Dw_->mult_ptr_EO(w_ptr, v_ptr);

    BGWilsonLA_MultScalar(w5_ptr,w_ptr,mass_fact,Nvol_);
    
    f5_ptr += f4size_;
    w5_ptr += f4size_;
  }
#else
  using namespace FieldExpression;
  Field lpf(f4size_),lmf(f4size_),w(f4size_),v(f4size_);
  double mass_fact= 4.0+M0_;

  for(int s=0; s<N5_; ++s) {
    proj_p(lpf,f5,(s+N5_-1)%N5_);
    if(s==0)     lpf *= -mq_;
    proj_m(lmf,f5,(s+1)%N5_);
    if(s==N5_-1) lmf *= -mq_;

    lpf += lmf;
    get4d(v,f5,s);
    v *= Params_.bs_[s];
    v += Params_.cs_[s]*lpf;
    w = Dw_->mult(v);
    w *= mass_fact;
    set5d(w5,w,s);
  }
#endif
}

void Dirac_optimalDomainWall::mult_dag_offdiag(Field& w5,const Field& f5) const{
  Field v5(f5size_),lpf(f4size_), lmf(f4size_);
  int spin_idx;
  double cs, bs;

#ifdef IBM_BGQ_WILSON
  Field w(f4size_);
  double* w_ptr = w.getaddr(0);

  /* here, Nvol_ is half-size */
  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    double* f5_ptr = const_cast<Field&>(f5).getaddr(spin_idx);
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);
    
    bs = (4.0+M0_)*Params_.bs_[s];
    cs = (4.0+M0_)*Params_.cs_[s];

    Dw_->mult_dag_ptr_EO(w_ptr,f5_ptr);
 
    BGWilsonLA_MultScalar(w5_ptr,w_ptr,bs,Nvol_);
    BGWilsonLA_MultScalar(v5_ptr,w_ptr,cs,Nvol_);
  }
  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);

    BGWilsonLA_Proj_P(lpf.getaddr(0),const_cast<Field&>(v5).getaddr(ff_.index(0,0,(s+1    )%N5_)),Nvol_);
    BGWilsonLA_Proj_M(lmf.getaddr(0),const_cast<Field&>(v5).getaddr(ff_.index(0,0,(s+N5_-1)%N5_)),Nvol_);
    
    if(s == N5_-1) BGWilsonLA_AXPBYPZ(w5_ptr,lpf.getaddr(0),lmf.getaddr(0),w5_ptr,-mq_, 1.0,Nvol_);
    else if(s==0)  BGWilsonLA_AXPBYPZ(w5_ptr,lpf.getaddr(0),lmf.getaddr(0),w5_ptr, 1.0,-mq_,Nvol_);
    else           BGWilsonLA_AXPBYPZ(w5_ptr,lpf.getaddr(0),lmf.getaddr(0),w5_ptr, 1.0, 1.0,Nvol_);
  }
#else
  assert(w5.size()==f5.size());

  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    double* f5_ptr = const_cast<Field&>(f5).getaddr(spin_idx);
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);
    
    bs = (4.0+M0_)*Params_.bs_[s];
    cs = (4.0+M0_)*Params_.cs_[s];
    
    Field w = Dw_->mult_dag(get4d(f5,s));
    double* w_ptr = w.getaddr(0);
    
    for (int i=0; i<f4size_; i++){
      w5_ptr[i] = bs*w_ptr[i];
      v5_ptr[i] = cs*w_ptr[i];
    }
  }
  for(int s=0; s<N5_; ++s){
    proj_p(lpf,v5,(s+1)%N5_);
    if(s == N5_-1) lpf *= -Params_.mq_;
    proj_m(lmf,v5,(s+N5_-1)%N5_);
    if(s == 0)     lmf *= -Params_.mq_;
    add5d(w5,lpf,lmf,s);
  }
#endif
}

const Field Dirac_optimalDomainWall::mult(const Field& f5) const{
  Field w5(f5size_);
  (this->*mult_core)(w5,f5);
  return w5;
}

const Field Dirac_optimalDomainWall::mult_dag(const Field& f5) const{
  Field w5(f5size_);
  (this->*mult_dag_core)(w5,f5);
  return w5;
}

/////// related useful functions  //////
const Field Dirac_optimalDomainWall::Dminus(const Field& f5) const{
  //1-c_s * D_w(-M)
  Field w5(f5size_);
  w5 = f5;
  for(int s=0; s<N5_; ++s) {
    Field temp =  Dw_->mult(get4d(f5,s));
    temp *= -Params_.cs_[s]; // = [-c_s * D_w(-M)]f5
    add5d(w5,temp,s); //= [1-c_s * D_w(-M)]f5
  }
  return w5;
}

const Field Dirac_optimalDomainWall::gamma5(const Field& f5) const{
  Field w5(f5size_); 
  
  for(int s=0; s<N5_; ++s){
    for(int site=0; site<Nvol_; ++site){
      gamma5core(w5.getaddr(ff_.index(0,site,s)),
		 const_cast<Field&>(f5).getaddr(ff_.index(0,site,s)));
    }
  }
  return w5; 
}

const Field Dirac_optimalDomainWall::R5(const Field& f5) const{
  Field w5(f5size_); 
  for(int s=0; s<N5_; ++s) set5d(w5,get4d(f5,s),N5_-s-1);
  return w5; 
}

const Field Dirac_optimalDomainWall::R5g5(const Field& f5) const{
  Field w5(f5size_);

  for(int s=0; s<N5_; ++s){
    for(int site=0; site<Nvol_; ++site){
      gamma5core(w5.getaddr(ff_.index(0,site,N5_-s-1)),
		 const_cast<Field&>(f5).getaddr(ff_.index(0,site,s)));
    }
  }
  return w5;
}

const Field Dirac_optimalDomainWall::Bproj( const Field& f5) const{ 
  Field f4(f4size_),t4(f4size_);

  proj_p(f4,f5,N5_-1);
  proj_m(t4,f5,0);
  f4 += t4;
  return f4;
}

const Field Dirac_optimalDomainWall::Bproj_dag(const Field& f4) const{
  Field f5(f5size_),t4(f4size_);
  proj_p(t4,f4,0);
  set5d(f5,t4,N5_-1);
  proj_m(t4,f4,0);
  set5d(f5,t4,0);
  return f5;
}

void Dirac_optimalDomainWall::proj_p(Field& w,const Field& f5,int s)const{
  for(int site=0; site<Nvol_; ++site)
    projPcore(w.getaddr(ff_.index(0,site)),
	      const_cast<Field&>(f5).getaddr(ff_.index(0,site,s)));
}

void Dirac_optimalDomainWall::proj_m(Field& w,const Field& f5,int s)const{
  for(int site=0; site<Nvol_; ++site)
    projMcore(w.getaddr(ff_.index(0,site)),
	      const_cast<Field&>(f5).getaddr(ff_.index(0,site,s)));
}

void Dirac_optimalDomainWall::gamma5_4d(Field& w,const Field& f)const{
  w = Dw_->gamma5(f);
}

//////////////////////////////////////////////////////////////////
/*! @brief contribution to the MD-force from forward difference */
void Dirac_optimalDomainWall::
md_force_p(Field& fce,const Field& phi,const Field& psi)const{
  using namespace FieldExpression;

  Field lpf(f4size_), lmf(f4size_);

  for(int s=0; s<N5_; ++s){
    proj_p(lpf,phi,(s+N5_-1)%N5_);
    if(s == 0)     lpf *= -mq_;
    proj_m(lmf,phi,(s+1    )%N5_);
    if(s == N5_-1) lmf *= -mq_;

    Field w = get4d(phi,s); 

    w *= Params_.bs_[s];
    w += Params_.cs_[s]*(lpf +lmf);

    Dw_->md_force_p(fce,w,get4d(psi,s));
  }
}  

/*! @brief contribution to the MD-force from backward difference */
void Dirac_optimalDomainWall::
md_force_m(Field& fce,const Field& phi,const Field& psi)const{
  using namespace FieldExpression;
  Field lpf(f4size_), lmf(f4size_);

  for(int s=0; s<N5_; ++s){
    proj_p(lpf,phi,(s+N5_-1)%N5_);
    if(s == 0)     lpf *= -mq_;
    proj_m(lmf,phi,(s+1    )%N5_);
    if(s == N5_-1) lmf *= -mq_;

    Field w = get4d(phi,s); 

    w *= Params_.bs_[s];
    w += Params_.cs_[s]*(lpf +lmf);

    Dw_->md_force_m(fce,w,get4d(psi,s));
  }
}  

/*! @brief total MD-force */
const Field Dirac_optimalDomainWall::
md_force(const Field& phi,const Field& psi) const{

  Field fce(Dw_->gsize());
  md_force_p(fce,phi,psi);
  md_force_m(fce,phi,psi);
  fce *= -0.5;
  return fce;
}

const Field Dirac_optimalDomainWall::get4d(const Field& f5,int s) const{
  Field w(f4size_);
  for (int i=0; i<f4size_; ++i) w.set(i, f5[s*f4size_+i]);
  return w;
}
void Dirac_optimalDomainWall::get4d(Field& f4,const Field& f5,int s) const{
  for (int i=0; i<f4size_; ++i) f4.set(i,f5[s*f4size_+i]);
}
void Dirac_optimalDomainWall::get4d_c(Field& f4,const Field& f5,const double& c,int s) const{
  double* f4_ptr = f4.getaddr(0);
  double* f5_ptr = const_cast<Field&>(f5).getaddr(s*f4size_);

  for (int i=0; i<f4size_; ++i) {
    f4_ptr[i] = c*f5_ptr[i];
  }
}
void Dirac_optimalDomainWall::set5d(Field& f5,const Field& f4,int s) const{
  for (int i=0; i<f4size_; ++i) f5.set(s*f4size_+i,f4[i]);
}
void Dirac_optimalDomainWall::set5d_c(Field& f5,const Field& f4,const double c,int s) const{
  double* f5_ptr = f5.getaddr(s*f4size_);
  double* f4_ptr = const_cast<Field&>(f4).getaddr(0);

  for (int i=0; i<f4size_; ++i){
    f5_ptr[i] = c*f4_ptr[i];
  }
}

void Dirac_optimalDomainWall::mul5d(Field& f5,double fac,int s) const{
  for(int i=0; i<f4size_; ++i) f5.set(s*f4size_+i,fac*f5[s*f4size_+i]);
}

void Dirac_optimalDomainWall::add5d(Field& f5,const Field& f4_1,const Field& f4_2,int s) const{
  for(int i=0; i<f4size_; ++i) f5.add(s*f4size_+i,f4_1[i]+f4_2[i]);
}
void Dirac_optimalDomainWall::add5d_c(Field& f5,const Field& f4,double c,int s) const{
  for(int i=0; i<f4size_; ++i) f5.add(s*f4size_+i,c*f4[i]);
}
void Dirac_optimalDomainWall::add5d_from5d(Field& f5,const Field& f,int s) const{
  for(int i=0; i<f4size_; ++i) f5.add(s*f4size_+i,f[s*f4size_+i]);
}
void Dirac_optimalDomainWall::add5d(Field& f5,const Field& f4,int s) const{
  f5.add(std::slice(s*f4size_,f4size_,1),f4.getva());
}

//
Preconditioner* Dirac_optimalDomainWall::
choose_Preconditioner(int PrecondID){
  switch (PrecondID){
  case NoPreconditioner:
    return new NoPrecond(this);
  case LUPreconditioner:
    return new LUPrecond(this);
  default:
    return new NoPrecond(this);
  }
}

/* @brief Namespace definining useful functions for DomainWallFermions*/
namespace DomainWallFermions {

  inline double set_vs( int is, int ns, double kprime ){
    double ekprime = gsl_sf_ellint_Kcomp( kprime , 0 ); 
    double vs = is * ekprime / ns; 
    return vs;
  }

  const vector<double> getOmega(int Ns,double lambda_min,double lambda_max){
    double u, m;
    double sn, cn, dn; 
    double kprime = sqrt(1.0-(lambda_min/lambda_max)*(lambda_min/lambda_max));
    
    vector<double> omegas(Ns);

    for(int ii=0; ii<Ns; ++ii){
      int is = 2*ii + 1;
      m = kprime*kprime;
      double vs = set_vs( is, Ns*2, kprime );
      gsl_sf_elljac_e( vs, m, &sn, &cn, &dn );
      double sn2 = sn * sn;
      double kappaprime2 = kprime * kprime;
      omegas[ii] = (1.0/lambda_min)* sqrt(1.0-kappaprime2*sn2);
    }
#if VERBOSITY>2
    for( int ii=0; ii<Ns; ++ii) printf("%24.16E\n", omegas[ii] );
#endif
    return omegas;
  }
}


