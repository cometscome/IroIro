/* !@filename dirac_wilson_Brillouin.cpp
 * @brief implementation of Dirac_Wilson_Brillouin class
 *  Time-stamp: <2013-08-26 06:33:04 noaki>
 */
#include "dirac_wilson_Brillouin.hpp"
#include "Tools/sunMatUtils.hpp"
#include "Tools/sunVec.hpp"
#include "Communicator/comm_io.hpp"
#include "Fields/field_expressions.hpp"

#ifdef IBM_BGQ_WILSON
#include "bgqwilson.h"
#include "dirac_wilson_Brillouin_BGQ.code"
#else
#include "dirac_wilson_Brillouin_regular.code"
#endif

using namespace SUNvecUtils;
using namespace std;

void (Dirac_Wilson_Brillouin::*Dirac_Wilson_Brillouin::gm[])
(double*,const double*) const = {&Dirac_Wilson_Brillouin::gammaXcore,
				 &Dirac_Wilson_Brillouin::gammaYcore,
				 &Dirac_Wilson_Brillouin::gammaZcore,
				 &Dirac_Wilson_Brillouin::gammaTcore,};

const Field Dirac_Wilson_Brillouin::mult(const Field& f)const{
  Field w(fsize_);
  (this->*mult_core)(w,f);
  return w;
}

const Field Dirac_Wilson_Brillouin::mult_dag(const Field& f)const{ 
  return gamma5(mult(gamma5(f)));
}

int Dirac_Wilson_Brillouin::sgm(int mu,int nu,int rho)const{
  for(int sigma=0; sigma<NDIM_; ++sigma){
    if(sigma !=mu && sigma !=nu && sigma !=rho) return sigma;
  }
}

const Field Dirac_Wilson_Brillouin::gamma5(const Field& f)const{ 
  Field w(fsize_);
  for(int site=0; site<Nvol_; ++site)
    gamma5core(w.getaddr(ff_.index(0,site)),
	       const_cast<Field&>(f).getaddr(ff_.index(0,site)));
  return w;
}

