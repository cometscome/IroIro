/*!
 * @file dirac_DomainWallCore_BGQ.hpp
 * @brief Declaration of class Dirac_optimalDomainWallCore_BGQ (5d operator)
 Time-stamp: <2014-01-24 14:45:30 neo>
 */
#ifndef DIRAC_OPTIMALDOMAINWALLCORE_BGQ_INCLUDED
#define DIRAC_OPTIMALDOMAINWALLCORE_BGQ_INCLUDED

#include "include/field.h"
#include "include/pugi_interface.h"
#include "Dirac_ops/dirac_wilson.hpp"
#include "Dirac_ops/domainWallUtils.hpp"

class DomainWallCore_BGQ {
private:
  const DomainWallParams& prms_;
  const DiracWilsonLike* Dw_; /*!< @brief Dirac Kernel - any WilsonLike op */ 
  ffmt_t ff_;
  int Nvol_,N5_,f4size_;
  int EOtag_;
  void(DomainWallCore_BGQ::*mult_core)(Field&,const Field&)const;
  void(DomainWallCore_BGQ::*mult_dag_core)(Field&,const Field&)const;
public:
  DomainWallCore_BGQ(const DomainWallParams& prms,
		     const DiracWilsonLike* Dw,int Nvol)
    :prms_(prms),Dw_(Dw),ff_(Nvol),Nvol_(Nvol),N5_(prms.N5_),
     EOtag_(0),f4size_(Dw->fsize()),
     mult_core(&DomainWallCore_BGQ::mult_full),
     mult_dag_core(&DomainWallCore_BGQ::mult_dag_full){assert(Dw_);}

  DomainWallCore_BGQ(const DomainWallParams& prms,const DiracWilsonLike* Dw,
		     int Nvol,DWFutils::EvenOdd_tag EO)
    :prms_(prms),Dw_(Dw),ff_(Nvol),Nvol_(Nvol),N5_(prms.N5_),
     EOtag_(EO.EOtag),f4size_(Dw->fsize()),
     mult_core(&DomainWallCore_BGQ::mult_offdiag),
     mult_dag_core(&DomainWallCore_BGQ::mult_dag_offdiag){assert(Dw_);}
  
  /*@! brief mult in the heavy M0 limit */
  const Field mult_hop5(const Field& f5)const;    
  const Field mult_hop5_inv(const Field& f5)const;
  const Field mult_hop5_dag(const Field& f5)const;
  const Field mult_hop5_dinv(const Field& f5)const;

  //  void Dminus(Field&,const Field&)const;
  const Field mult(const Field& f5)const;
  const Field mult_dag(const Field& f5)const;
  void mult_offdiag(Field&,const Field&)const;    /*! @brief  -kpp*D*f    */
  void mult_full(Field&,const Field&)const;       /*! @brief  (1-kpp*D)*f */
  void mult_dag_offdiag(Field&,const Field&)const;/*! @brief -kpp*D^dag*f    */
  void mult_dag_full(Field&,const Field&)const;   /*! @brief (1-kpp*D^dag)*f */

  void md_force_p(Field&,const Field&,const Field&)const;
  void md_force_m(Field&,const Field&,const Field&)const;
  const Field md_force(const Field& phi,const Field& psi)const;
};

#endif
