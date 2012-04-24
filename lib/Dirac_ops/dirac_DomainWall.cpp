/*!--------------------------------------------------------------------------
 * @file dirac_DomainWall.cpp
 *
 * @brief Definition of class methods for Dirac_optimalDomainWall (5d operator)
 *
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

using namespace std;

// Constructors for DomainWall Parameters classes
//======================================================================
Dirac_optimalDomainWall_params::
Dirac_optimalDomainWall_params(XML::node DWF_node,DWFType Type){
  std::string Precond_string;
  XML::read(DWF_node, "Preconditioning", Precond_string, MANDATORY);
  XML::read(DWF_node, "N5d", N5_, MANDATORY);
  XML::read(DWF_node, "wilson_mass", M0_, MANDATORY);
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
  using namespace FieldExpression;

  assert(f5.size()==fsize_);
  Field w5(fsize_);

  for(int s=0; s<N5_; ++s) {
    Field v = get4d(f5,s);
    v *= Params.dp_[s];
    set5d(w5,v,s);
  }
  for(int s=0; s<N5_-1; ++s) {
    Field v = proj_m(get4d(f5,s+1));
    v *= -Params.dm_[s];
    add5d(w5,v,s);
  }
  for(int s=1; s<N5_; ++s) {
    Field v = proj_p(get4d(f5,s-1));
    v *= -Params.dm_[s];
    add5d(w5,v,s);
  }

  Field v = proj_p(get4d(f5,N5_-1));
  v *= mq_*Params.dm_[0];
  add5d(w5,v,0);

  v = proj_m(get4d(f5,0));
  v *= mq_*Params.dm_[N5_-1];
  add5d(w5,v,N5_-1);

  return w5;
}

const Field Dirac_optimalDomainWall::mult_hop5_dag(const Field& f5) const{

  assert(f5.size()==fsize_);
  Field w5(fsize_);

  for(int s=0; s<N5_; ++s){
    Field v = get4d(f5,s);
    v *= Params.dp_[s];
    set5d(w5,v,s);
  }
  for(int s=0; s<N5_-1; ++s){
    Field v = proj_p(get4d(f5,s+1));
    v *= -Params.dm_[s+1];
    add5d(w5,v,s);
  }
  for(int s=1; s<N5_; ++s){
    Field v = proj_m(get4d(f5,s-1));
    v *= -Params.dm_[s-1];
    add5d(w5,v,s);
  }
  Field v = proj_m(get4d(f5,N5_-1));
  v *= mq_*Params.dm_[N5_-1];
  add5d(w5,v,0);

  v = proj_p(get4d(f5,0));
  v *= mq_*Params.dm_[0];
  add5d(w5,v,N5_-1);

  return w5;
}

const Field Dirac_optimalDomainWall::mult_hop5_inv(const Field& f5) const{
  assert(f5.size()==fsize_);
  Field w5(f5);

  for (int s=1; s<N5_; ++s) {
    Field lpf =  proj_p(get4d(w5,s-1));
    lpf *= (Params.dm_[s]/Params.dp_[s-1]);
    add5d(w5,lpf,s);
  }
  for (int s=0; s< N5_-1; ++s) {
    Field ey = proj_m(get4d(w5,s));
    ey *= -mq_* Params.es_[s];
    add5d(w5,ey,N5_-1);
  }

  Field v =  get4d(w5,N5_-1);
  v *= 1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]);
  set5d(w5,v,N5_-1);

  for(int s=N5_-2; s>=0; --s) {
    Field lmf = proj_m(get4d(w5,s+1));
    lmf *= Params.dm_[s];
    add5d(w5,lmf,s);
    Field fy = proj_p(get4d(w5,N5_-1));
    fy *= -mq_*Params.fs_[s];
    add5d(w5,fy,s);
    v = get4d(w5,s);
    v*= 1.0/ Params.dp_[s];
    set5d(w5,v,s);
  }
  return w5;
}

const Field Dirac_optimalDomainWall::mult_hop5_dinv(const Field& f5) const{

  assert(f5.size()==fsize_);   
  Field w5(f5);

  Field v = get4d(w5,0);
  v *= 1.0/Params.dp_[0];
  set5d(w5,v,0);

  for(int s=1; s<N5_-1; ++s){
    Field lmf = proj_m(get4d(w5,s-1));                                         
    lmf *= Params.dm_[s-1];                                                     
    add5d(w5,lmf,s);                                                            
    v = get4d(w5,s);                                                            
    v*= 1.0/Params.dp_[s];                                                      
    set5d(w5,v,s);                                                              
  }                                                                             
  v = proj_m(get4d(w5,N5_-2));                                                  
  v*= Params.dm_[N5_-2];                                                        
  add5d(w5,v,N5_-1);                                                            
  for(int s=0; s<N5_-1; ++s) {                                                  
    Field fy = proj_p(get4d(w5,s));                                             
    fy *= -mq_*Params.fs_[s];                                            
    add5d(w5,fy,N5_-1);                                                         
  }                                                                             
  v = get4d(w5,N5_-1);                                                          
  v*= 1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]);    
  set5d(w5,v,N5_-1);                                                            
                                                                                
  for(int s=N5_-2; s>=0; --s){                                                  
    Field lpf = proj_p(get4d(w5,s+1));                                          
    lpf *= (Params.dm_[s+1]/Params.dp_[s]);                                     
    add5d(w5,lpf,s);                                                            
    Field ey = proj_m(get4d(w5,N5_-1));                                         
    ey *= -mq_*Params.es_[s];                                            
    add5d(w5,ey,s);                                                             
  }                                                                             
  return w5;    
}

/*! @brief definitions of D_dwf */
void Dirac_optimalDomainWall::mult_full(Field& w5, const Field& f5) const{ 
  using namespace FieldExpression;
  //assert(w5.size()==f5.size());
  
  Field v(f4size_),lpf(f4size_), lmf(f4size_);
  
  double* v_ptr   = v.getaddr(0);
  double* lpf_ptr = lpf.getaddr(0);
  double* lmf_ptr = lmf.getaddr(0);

  double mass_fact= 4.0+M0_;

  for(int s=0; s<N5_; ++s) {
    double* f5_ptr = const_cast<Field&>(f5).getaddr(s*f4size_);
    double* w5_ptr   = w5.getaddr(s*f4size_);
    proj_p(lpf,f5,(s+N5_-1)%N5_);
    if(s==0)     lpf *= -mq_;
    proj_m(lmf,f5,(s+1)%N5_);
    if(s==N5_-1) lmf *= -mq_;

    for (int i=0; i<f4size_; ++i) {
      lpf_ptr[i] += lmf_ptr[i];
      v_ptr[i] = Params.bs_[s]*f5_ptr[i]+Params.cs_[s]*lpf_ptr[i];
    }

    Field w = Dw_.mult(v);
    double* w_ptr = w.getaddr(0);

    for (int i=0; i<f4size_; ++i) {
      w5_ptr[i] = mass_fact*w_ptr[i]+ f5_ptr[i] - lpf_ptr[i];
    }

  }
}

void Dirac_optimalDomainWall::mult_dag_full(Field& w5,const Field& f5) const{
  //assert(w5.size()==f5.size());
  Field v5(fsize_);
  Field lpf(f4size_), lmf(f4size_);

  int spin_idx;
  double cs, bs;
  
  for(int s=0; s< N5_; ++s){
    spin_idx = s*f4size_;
    double* f5_ptr = const_cast<Field&>(f5).getaddr(spin_idx);
    double* w5_ptr = w5.getaddr(spin_idx);
    double* v5_ptr = v5.getaddr(spin_idx);
    
    bs = (4.0+M0_)*Params.bs_[s];
    cs = (4.0+M0_)*Params.cs_[s];
    Field w = Dw_.mult_dag(get4d(f5,s));
    double* w_ptr = w.getaddr(0);
    
    for (int i=0; i<f4size_; i++){
      w5_ptr[i] = bs*w_ptr[i];
      v5_ptr[i] = cs*w_ptr[i];
    }
    
    // do not change this
    for (int i=0; i<f4size_; i++){
      w5_ptr[i] += f5_ptr[i];
      v5_ptr[i] -= f5_ptr[i];
    }
  }
  

  for(int s = 0; s < N5_; ++s){
    proj_p(lpf,v5,(s+1)%N5_);
    if(s == N5_-1) lpf *= -mq_;
    proj_m(lmf,v5,(s+N5_-1)%N5_);
    if(s == 0)     lmf *= -mq_;
    add5d(w5,lpf,lmf,s);
  }
}

void Dirac_optimalDomainWall::mult_offdiag(Field& w5, const Field& f5) const{ 
  using namespace FieldExpression;
  assert(w5.size()==f5.size());

  for(int s=0; s<N5_; ++s) {
    Field lpf = proj_p(get4d(f5,(s+N5_-1)%N5_));
    if(s==0)     lpf *= -mq_;
    Field lmf = proj_m(get4d(f5,(s+1)%N5_));
    if(s==N5_-1) lmf *= -mq_;

    Field v = get4d(f5,s);
    v *= Params.bs_[s];
    v += Params.cs_[s]*(lpf +lmf);
    Field w = Dw_.mult(v);
    w *= 4.0+M0_;
    set5d(w5,w,s);
  }
}

void Dirac_optimalDomainWall::mult_dag_offdiag(Field& w5,const Field& f5) const{
  assert(w5.size()==f5.size());
  Field v5(fsize_);
  
  for(int s=0; s<N5_; ++s){
    Field dv = Dw_.mult_dag(get4d(f5,s));
    dv *= (4.0+M0_)*Params.bs_[s];
    set5d(w5,dv,s);
    dv *= Params.cs_[s]/Params.bs_[s];
    set5d(v5,dv,s);
  }
  for(int s = 0; s < N5_; ++s){
    Field lpf = proj_p(get4d(v5,(s+1)%N5_));
    if(s == N5_-1) lpf *= -Params.mq_;
    Field lmf = proj_m(get4d(v5,(s+N5_-1)%N5_));
    if(s == 0)     lmf *= -Params.mq_;
    add5d(w5,lpf,s);
    add5d(w5,lmf,s);
  }
}

const Field Dirac_optimalDomainWall::mult(const Field& f5) const{
  Field w5(fsize_);
  (this->*mult_core)(w5,f5);
  return w5;
}

const Field Dirac_optimalDomainWall::mult_dag(const Field& f5) const{
  Field w5(fsize_);
  (this->*mult_dag_core)(w5,f5);
  return w5;
}

/////// related useful functions  //////
const Field Dirac_optimalDomainWall::Dminus(const Field& f5) const{
  //1-c_s * D_w(-M)
  Field w5(fsize_);
  w5 = f5;
  for(int s=0; s<N5_; ++s) {
    Field temp =  Dw_.mult(get4d(f5,s));
    temp *= -Params.cs_[s]; // = [-c_s * D_w(-M)]f5
    add5d(w5, temp, s); //= [1-c_s * D_w(-M)]f5
  }
  return w5;
}

const Field Dirac_optimalDomainWall::gamma5(const Field& f5) const{
  Field w5(fsize_); 
  for(int s=0; s<N5_; ++s) set5d(w5,gamma5_4d(get4d(f5,s)),s);
  return w5; 
}

const Field Dirac_optimalDomainWall::R5(const Field& f5) const{
  Field w5(fsize_); 
  for(int s=0; s<N5_; ++s) set5d(w5,get4d(f5,s),N5_-s-1);
  return w5; 
}

const Field Dirac_optimalDomainWall::R5g5(const Field& f5) const{
  //return R5(gamma5(f5)); 
  Field w5(fsize_);
  for(int s=0; s<N5_; ++s) set5d(w5,gamma5_4d(get4d(f5,s)),N5_-s-1);
  return w5;
}

const Field Dirac_optimalDomainWall::Bproj( const Field& f5) const{ 
  Field f4 = proj_p(get4d(f5,N5_-1));
  f4 += proj_m(get4d(f5,0));
  return f4;
}

const Field Dirac_optimalDomainWall::Bproj_dag(const Field& f4) const{
  Field f5(fsize_);
  set5d(f5,proj_p(f4),N5_-1);
  set5d(f5,proj_m(f4),0);
  return f5;
}

const Field Dirac_optimalDomainWall::proj_p(const Field& f4) const{
  Field w4 = Dw_.proj_p(f4);
  return w4;
}

const Field Dirac_optimalDomainWall::proj_m(const Field& f4) const{
  Field w4 = Dw_.proj_m(f4);
  return w4;
}

void Dirac_optimalDomainWall::proj_p(Field& w, const Field& f, int s) const{
  Dw_.proj_p(w,f,s);
}
void Dirac_optimalDomainWall::proj_m(Field& w, const Field& f, int s) const{
  Dw_.proj_m(w,f,s);
}


//////////////////////////////////////////////////////////////////
/*! @brief contribution to the MD-force from forward difference */
void Dirac_optimalDomainWall::
md_force_p(Field& fce,const Field& phi,const Field& psi)const{
  using namespace FieldExpression;

  for(int s=0; s<N5_; ++s){
    Field lpf = proj_p(get4d(phi,(s+N5_-1)%N5_));
    if(s == 0)     lpf *= -mq_;
    Field lmf = proj_m(get4d(phi,(s+1    )%N5_));
    if(s == N5_-1) lmf *= -mq_;

    Field w = get4d(phi,s); 

    w *= Params.bs_[s];
    w += Params.cs_[s]*(lpf +lmf);

    Dw_.md_force_p(fce,w,get4d(psi,s));
  }
}  

/*! @brief contribution to the MD-force from backward difference */
void Dirac_optimalDomainWall::
md_force_m(Field& fce,const Field& phi,const Field& psi)const{
  using namespace FieldExpression;

  for(int s=0; s<N5_; ++s){
    Field lpf = proj_p(get4d(phi,(s+N5_-1)%N5_));
    if(s == 0)     lpf *= -mq_;
    Field lmf = proj_m(get4d(phi,(s+1    )%N5_));
    if(s == N5_-1) lmf *= -mq_;

    Field w = get4d(phi,s); 

    w *= Params.bs_[s];
    w += Params.cs_[s]*(lpf +lmf);

    Dw_.md_force_m(fce,w,get4d(psi,s));
  }
}  

/*! @brief total MD-force */
const Field Dirac_optimalDomainWall::
md_force(const Field& phi,const Field& psi) const{

  Field fce(gsize_);
  md_force_p(fce,phi,psi);
  md_force_m(fce,phi,psi);
  fce *= -0.5;
  return fce;
}

const Field Dirac_optimalDomainWall::get4d(const Field& f5,int s) const{
  Field w(f4size_);
  for (int i=0; i<f4size_; ++i) w.set(i, f5[s*f4size_+i]);
  return w;
  //  return Field(f5[std::slice(s*f4size_,f4size_,1)]);
}
void Dirac_optimalDomainWall::get4d(Field& f4,const Field& f5,int s) const{
  for (int i=0; i<f4size_; ++i) f4.set(i,f5[s*f4size_+i]);
}
void Dirac_optimalDomainWall::get4d_c(Field& f4,const Field& f5,const double& c,int s) const{
  double* f4_ptr = f4.getaddr(0);
  double* f5_ptr = const_cast<Field&>(f5).getaddr(s*f4size_);

  for (int i=0; i<f4size_; ++i) {
    //f4.set(i,c*f5[s*f4size_+i]);
    f4_ptr[i] = c*f5_ptr[i];
  }
}
void Dirac_optimalDomainWall::set5d(Field& f5,const Field& f4,int s) const{
  for (int i=0; i<f4size_; i++) f5.set(s*f4size_+i,f4[i]);
}
void Dirac_optimalDomainWall::set5d_c(Field& f5,const Field& f4,const double c,int s) const{
  double* f5_ptr = f5.getaddr(s*f4size_);
  double* f4_ptr = const_cast<Field&>(f4).getaddr(0);

  for (int i=0; i<f4size_; i++){
    //f5.set(s*f4size_+i,c*f4[i]);
    f5_ptr[i] = c*f4_ptr[i];
  }
}
void Dirac_optimalDomainWall::add5d(Field& f5,const Field& f4_1, const Field& f4_2,int s) const{
  for (int i=0; i<f4size_; i++) f5.add(s*f4size_+i,f4_1[i]+f4_2[i]);
}
void Dirac_optimalDomainWall::add5d_c(Field& f5,const Field& f4, double c,int s) const{
  for (int i=0; i<f4size_; i++) f5.add(s*f4size_+i,c*f4[i]);
}
void Dirac_optimalDomainWall::add5d_from5d(Field& f5,const Field& f,int s) const{
  for (int i=0; i<f4size_; i++) f5.add(s*f4size_+i,f[s*f4size_+i]);
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

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/*
  @brief Namespace definining useful functions for DomainWallFermions
*/
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
