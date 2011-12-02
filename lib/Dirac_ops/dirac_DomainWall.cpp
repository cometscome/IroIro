/*!
 * @file dirac_DomainWall.cpp
 *
 * @brief Definition of class methods for Dirac_optimalDomainWall (5d operator)
 */
#include "dirac_DomainWall.hpp"
#include<stdlib.h>
#include<stdio.h>
#include <cassert>

#include<math.h>

#include<gsl/gsl_sf_ellint.h>
#include<gsl/gsl_sf_elljac.h>

using namespace std;

const Field Dirac_optimalDomainWall::NoPrecond::mult(const Field& f5) const{
  return DWF_->mult(f5);
}

const Field Dirac_optimalDomainWall::LUPrecond::mult(const Field& f5) const{
  using namespace FieldExpression;
  Field w5(DWF_->fsize_);
  w5 = DWF_->mult(f5);
  
  for (int s=1; s<DWF_->N5_; ++s) {
    Field lpf = DWF_->proj_p(DWF_->get4d(w5,s-1));
    lpf *= (DWF_->Params.dm_[s]/DWF_->Params.dp_[s-1]);
    DWF_->add5d(w5,lpf,s);
  };
  Field v = DWF_->get4d(w5,DWF_->N5_-1);
  v *= 1.0/DWF_->Params.dp_[DWF_->N5_-1];
  DWF_->set5d(w5,v,DWF_->N5_-1);
  for (int s=DWF_->N5_-2; s>=0; --s) {
    Field lmf = DWF_->proj_m(DWF_->get4d(w5,s+1));
    lmf *= DWF_->Params.dm_[s];
    DWF_->add5d(w5,lmf,s);
    v = DWF_->get4d(w5,s);
    v *= 1.0/DWF_->Params.dp_[s];
    DWF_->set5d(w5,v,s);
    
  }
  return w5;
}


const Field Dirac_optimalDomainWall::mult(const Field& f5) const{ 
  using namespace FieldExpression;

  assert(f5.size()==fsize_);
  Field w5(fsize_);

  for(int s=0; s<N5_; ++s) {
    Field lpf = proj_p(get4d(f5,(s+N5_-1)%N5_));
    if(s == 0)     lpf *= -Params.mq_;
    Field lmf = proj_m(get4d(f5,(s+1)%N5_));
    if(s == N5_-1) lmf *= -Params.mq_;
      
    Field w = get4d(f5,s);
    w -= lpf;
    w -= lmf;
      
    Field v = get4d(f5,s);
    v += (Params.cs_[s]/Params.bs_[s])*lpf;
    v += (Params.cs_[s]/Params.bs_[s])*lmf;

    w += (4.0+M0_)*Params.bs_[s]*(Dw_->mult(v));      
    set5d(w5,w,s);
    }
  
  return w5;
}


const Field Dirac_optimalDomainWall::mult_dag(const Field& f5) const{
  //  using namespace FieldExpression;

  assert(f5.size()==fsize_);
  Field v5(fsize_);
  Field w5(fsize_);

  Field t5(fsize_);
  t5 = f5;
  
  if (Params.Preconditioned_) {
    // precond LU: ((LU)^T)^-1 = (U^T L^T)^-1 = (L^T)^-1 (U^T)^-1
    Field v = get4d(t5,0);
    v *= 1.0/Params.dp_[0];
    set5d(t5,v,0);
    for (int s=1; s<N5_; ++s) {
      Field lmf = proj_m(get4d(t5,s-1));
      lmf *= Params.dm_[s-1];
      add5d(t5,lmf,s);
      v = get4d(t5,s);
      v *= 1.0/Params.dp_[s];
      set5d(t5,v,s);
    }
    for (int s=N5_-2; s>=0; --s) {
      Field lpf = proj_p(get4d(t5,s+1));
      lpf *= (Params.dm_[s+1]/Params.dp_[s]);
      add5d(t5,lpf,s);
    }
    // end precond LU
  }

  for(int s=0; s<N5_; ++s){
    Field dv = Dw_->mult_dag(get4d(t5,s));
    dv *= (4.0+M0_)*Params.bs_[s];
    set5d(w5,dv,s);
    dv *= (Params.cs_[s]/Params.bs_[s]);
    set5d(v5,dv,s);
  }
  w5 += t5;
  v5 -= t5;

  for(int s = 0; s < N5_; ++s){
    Field lpf = proj_p(get4d(v5,(s+1)%N5_));
    if(s == N5_-1) lpf *= -Params.mq_;
    Field lmf = proj_m(get4d(v5,(s+N5_-1)%N5_));
    if(s == 0)     lmf *= -Params.mq_;
    add5d(w5,lpf,s);
    add5d(w5,lmf,s);

  }
  return w5;
}

const Field Dirac_optimalDomainWall::
md_force(const Field& phi,const Field& psi) const{

  using namespace FieldExpression;

  Field w5(fsize_);
  for(int s = 0; s < N5_; ++s){

    Field lpf = proj_p(get4d(phi,(s +1)%N5_));
    if(s == N5_-1) lpf *= -Params.mq_;
    Field lmf = proj_m(get4d(phi,(s +N5_-1)%N5_));
    if(s == 0) lmf *= -Params.mq_;

    Field w4 = Field(get4d(phi,s));
    w4 += Params.c_*lpf;
    w4 += Params.c_*lmf;
    
    set5d(w5,w4,s);
  }

  Field force(gsize_);
  for(int s = 0; s < N5_; ++s)
    force += (4.0+M0_)*Params.omega_[s]*Dw_->md_force(get4d(w5,s),get4d(psi,s));

  return force;
}

const Field Dirac_optimalDomainWall::gamma5(const Field& f5) const{
  Field w5(fsize_); 
  for(int s = 0; s < N5_; ++s) set5d(w5,gamma5_4d(get4d(f5,s)),s);
  return w5; 
}

const Field Dirac_optimalDomainWall::R5(const Field& f5) const{
  Field w5(fsize_); 
  for(int s = 0; s < N5_; ++s) set5d(w5,get4d(f5,s),N5_-s-1);
  return w5; 
}

const Field Dirac_optimalDomainWall::R5g5(const Field& f5) const{
  return R5(gamma5(f5)); 
}

const Field Dirac_optimalDomainWall::Bproj( const Field& f5) const{ 
  Field f4 = proj_p(get4d(f5,N5_-1));
  f4 += proj_m(get4d(f5,0));
  //  Field f4 = proj_p(get4d(f5,0));
  //  f4 += proj_m(get4d(f5,N5_-1));
  return f4;
}

const Field Dirac_optimalDomainWall::Bproj_dag(const Field& f4) const{
  Field f5(fsize_);
  set5d(f5,proj_p(f4),N5_-1);
  set5d(f5,proj_m(f4),0);
  //  set5d(f5,proj_p(f4),0);
  //  set5d(f5,proj_m(f4),N5_-1);
  return f5;
}

namespace DomainWallFermions {

  inline double set_vs( int is , int ns , double kprime )
  {
    double ekprime = gsl_sf_ellint_Kcomp( kprime , 0 ); 
    double vs = is * ekprime / ns; 
    return vs;
  }
  

  vector<double> getOmega(int Ns, 
			  double lambda_min, 
			  double lambda_max)
  {
    double u, m;
    double sn , cn , dn; 
    double kprime = sqrt( 1.0 - (lambda_min/lambda_max) *
			  (lambda_min/lambda_max));
    
    vector<double> omegas(Ns);

    for( int ii = 0 ; ii < Ns ; ii ++ )
      {
	int is = 2 * ii + 1;
	m = kprime * kprime;
	double vs = set_vs( is , Ns*2 , kprime );
	gsl_sf_elljac_e( vs , m , &sn , &cn , &dn );
	double sn2 = sn * sn;
	double kappaprime2 = kprime * kprime;
	omegas[ii] = ( 1.0 / lambda_min ) * sqrt( 1.0 - kappaprime2 * sn2 );
      }
    
    #ifdef VERBOSE2
    for( int ii = 0 ; ii < Ns ; ii ++ )
      printf("%24.16E\n", omegas[ii] );
    #endif
    return omegas;
  }
  

  
}
