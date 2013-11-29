//--------------------------------------------------------------
/*! @file dirac_wilson_adjoint_EvenOdd.cpp
 *  @brief Definition of Even Odd wilson operator
*/
//--------------------------------------------------------------
#include "dirac_wilson_adjoint_EvenOdd.hpp"

using namespace std;

const Field Dirac_Wilson_Adjoint_EvenOdd::mult_eo(const Field& f) const{ 
  return Deo_.mult(f);
}

const Field Dirac_Wilson_Adjoint_EvenOdd::mult_oe(const Field& f) const{
  return Doe_.mult(f);
}

const Field Dirac_Wilson_Adjoint_EvenOdd::mult_eo_dag(const Field& f) const{
  return Doe_.mult_dag(f);
}

const Field Dirac_Wilson_Adjoint_EvenOdd::mult_oe_dag(const Field& f) const{
  return Deo_.mult_dag(f);
}

const Field Dirac_Wilson_Adjoint_EvenOdd::mult(const Field& f) const{
  Field w(f);
  w -= Deo_.mult(Doe_.mult(f));
  return w;
}

const Field Dirac_Wilson_Adjoint_EvenOdd::mult_dag(const Field& f) const{
  //return gamma5(mult(gamma5(f)));
  Field w(f);
  w -= mult_oe_dag(mult_eo_dag(f));
  return w;
}

void Dirac_Wilson_Adjoint_EvenOdd::
md_force_eo(Field& fce, const Field& eta,const Field& zeta) const{
  Deo_.md_force_p(fce,eta,zeta);
  Doe_.md_force_m(fce,eta,zeta);
}
void Dirac_Wilson_Adjoint_EvenOdd::
md_force_oe(Field& fce, const Field& eta,const Field& zeta) const{
  Doe_.md_force_p(fce,eta,zeta);
  Deo_.md_force_m(fce,eta,zeta);
}

const Field Dirac_Wilson_Adjoint_EvenOdd::
md_force(const Field& eta,const Field& zeta) const{
  Field fce(gsize());
  md_force_eo(fce,mult_oe(eta),zeta);
  md_force_oe(fce,eta,mult_eo_dag(zeta));
  fce *= getKappa();

  return fce;
}

