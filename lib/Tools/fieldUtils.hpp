/*! @file fieldUtils.hpp
    @brief header file for utility functions for GeneralField
 */
#ifndef FIELDUTILS_INCLUDED
#define FIELDUTILS_INCLUDED

#include "sunVec.hpp"
#include "sunAdjMat.hpp"
#include "sunAdjVec.hpp"
#include "include/common_fields.hpp"
#include "Geometry/siteIndex_EvenOdd.hpp"

class RandNum;

namespace FieldUtils{
  const GaugeField TracelessAntihermite(const GaugeField&);
  const GaugeField1D TracelessAntihermite(const GaugeField1D&);
  const GaugeField ReUnit(const GaugeField&);
  const GaugeField1D ReUnit(const GaugeField1D&);
  const GaugeField Exponentiate(const GaugeField&, const double, const int);

  const GaugeField1D field_oprod(const FermionField&,const FermionField&);

  void RandomGauge(GaugeField1D& G,const RandNum& rand);
  void RandomGauge(GaugeField& G,const RandNum& rand);

  // Field type-type transformations
  GaugeField1D DirSlice(const GaugeField& F, int dir);

#if defined IBM_BGQ_WILSON || defined SR16K_WILSON
  void DirSliceALINE(GaugeField1D &G,const GaugeField& F,int dir);

  inline double ReTrALINE(double& result,const double *F,int site,int dir,int Nvol){
    //assumes some indexing and SU(3) matrix
    result += F[    18*(site+Nvol*dir)];
    result += F[ 8 +18*(site+Nvol*dir)];
    result += F[16 +18*(site+Nvol*dir)];
  }
#endif

#ifdef IBM_BGQ_WILSON
  void ExponentiateMult_BGQ(GaugeField& F,const GaugeField&,double,int);
#endif

#ifdef SR16K_WILSON
  void ExponentiateMult_SR16K(GaugeField& F,const GaugeField&,double,int);
#endif

  void SetSlice(GaugeField&, const GaugeField1D&, int dir);
  void AddSlice(GaugeField&, const GaugeField1D&, int dir);

  void SetMat(GaugeField& F, const SUNmat& mat, int site, int dir);
  void SetMat(GaugeField1D& F, const SUNmat& mat, int site);
  
  void AddMat(GaugeField& F, const SUNmat& mat, int site, int dir);
  void AddMat(GaugeField1D& F, const SUNmat& mat, int site);

  void SetVec(FermionField&, const SUNvec&, int spin, int site); 
  void SetVec(FermionField1sp&, const SUNvec&, int site); 

  void AddVec(FermionField&, const SUNvec&, int spin, int site); 
  void AddVec(FermionField1sp&, const SUNvec&, int site); 

  // Inline functions
  inline SUNmat mat(const GaugeField& F,int site,int dir){
    return SUNmat(F.data[F.format.islice(site,dir)]);  }

  inline SUNmat mat(const GaugeField1D& F,int site){
    return SUNmat(F.data[F.format.islice(site)]);  }

  inline SUNmat mat_dag(const GaugeField& F,int site,int dir){
    return SUNmat(F.data[F.format.islice(site,dir)]).dag();  }

  inline SUNmat mat_dag(const GaugeField1D& F,int site){
    return SUNmat(F.data[F.format.islice(site)]).dag();  }

  inline SUNvec vec(const FermionField& F,int spin,int site){
    return SUNvec(F.data[F.format.cslice(spin,site)]);  }

  inline SUNvec vec(const FermionField1sp& F,int site){
    return SUNvec(F.data[F.format.islice(site)]);  }
  //
  inline SUNadjMat mat(const AdjGaugeField& F,int site,int dir){
    return SUNadjMat(F.data[F.format.islice(site,dir)]);  }

  inline SUNadjMat mat_dag(const AdjGaugeField& F,int site,int dir){
    return SUNadjMat(F.data[F.format.islice(site,dir)]).dag();  }

  inline SUNadjVec vec(const AdjFermionField1sp& F,int site){
    return SUNadjVec(F.data[F.format.islice(site)]);  }
  
  // function templates
  template<typename GF>
  GF get_even(const GF& Fin){
    GF Fout(Fin.Nvol()/2);
    Fout.data = Fin.data[Fin.get_sub(SiteIndex_EvenOdd::instance()->esec())];
    return Fout;
  }

  template<typename GF>
  GF get_odd(const GF& Fin){
    GF Fout(Fin.Nvol()/2);
    Fout.data = Fin.data[Fin.get_sub(SiteIndex_EvenOdd::instance()->osec())];
    return Fout;
  }

  template<typename GF>
  GF combine_eo(const GF& Fe,const GF& Fo){
    GF Fout(Fe.Nvol()+Fo.Nvol());
    Fout.data.set(Fout.get_sub(SiteIndex_EvenOdd::instance()->esec()),Fe.getva());
    Fout.data.set(Fout.get_sub(SiteIndex_EvenOdd::instance()->osec()),Fo.getva());
    return Fout;
  }
}

#endif
