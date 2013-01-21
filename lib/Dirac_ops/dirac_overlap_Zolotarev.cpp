//--------------------------------------------------------------------
// dirac_overlap_Zolotarev.cpp
//--------------------------------------------------------------------
#include "dirac_overlap_Zolotarev.hpp"
#include "Fields/field_expressions.hpp"
#include "Tools/randNum_MP.h"
#include "Main/Geometry/siteIndex.hpp"
using namespace std;

void Dirac_overlap_Zolotarev::
get_RandGauss(std::valarray<double>& phi,const RandNum& rng)const{
  Format::Format_F ff(Fopr_signH->Nvol()); // temporal hack
  MPrand::mp_get_gauss(phi,rng,SiteIndex::instance()->get_gsite(),ff);
}

const Field Dirac_overlap_Zolotarev::mult(const Field& f) const{
  using namespace FieldExpression;

  cout << "Dirac_overlap::mult called"<<endl;
  Field v = Fopr_signH->gamma5(Fopr_signH->mult(f));

  v *= Params.M0-0.5*Params.mq;
  v += (Params.M0+0.5*Params.mq)*f;
  return v;
}

const Field Dirac_overlap_Zolotarev::mult_dag(const Field& f) const{ 
  //return Fopr_signH->gamma5(Fopr_signH->mult(Fopr_signH->gamma5(f)));
  return Fopr_signH->gamma5(mult(Fopr_signH->gamma5(f)));
}

const Field Dirac_overlap_Zolotarev::gamma5(const Field& f) const{ 
  return Fopr_signH->gamma5(f);
}

const Field Dirac_overlap_Zolotarev::
md_force(const Field& eta,const Field& zeta) const{ 
  Field fce(Fopr_signH->gsize());
  Fopr_signH->calc_force(fce,eta,zeta);
}


