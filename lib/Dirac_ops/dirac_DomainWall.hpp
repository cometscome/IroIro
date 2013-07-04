/*!
 * @file dirac_DomainWall.hpp
 * @brief Declaration of class Dirac_optimalDomainWall (5d operator)
 Time-stamp: <2013-07-04 11:23:52 noaki>
 */
#ifndef DIRAC_OPTIMALDOMAINWALL_INCLUDED
#define DIRAC_OPTIMALDOMAINWALL_INCLUDED

#include <string>
#include <string.h>
#include "include/field.h"
#include "include/macros.hpp"

#include "include/pugi_interface.h"

#include "dirac_wilson.hpp"

static double wilson_mult_timer;
static double wilson_multdag_timer;

#ifdef IBM_BGQ_WILSON
struct SolverOutput;
#endif

namespace DomainWallFermions {
  struct EvenOdd_tag{};
  const std::vector<double> getOmega(int Ns,double lmd_min,double lmd_max);
  double read_wilson_mass(const XML::node& node);
}

enum DWFType {Regular, PauliVillars};
/*!
 * @brief Container for parameter of the 5d Optimal Domain Wall operator
 * Parameters for \f[D_{\rm dwf}(m_q) =\omega D_W(-m_0)(1+cL(m_q))+(1-L(m_q))\f]
 */
struct Dirac_optimalDomainWall_params{
  double N5_;/*!< @brief the length in the 5th direction (must be even) */
  double b_; /*!< @brief scale factor (b!=1 for scaled Shamir H_T) */
  double c_; /*!< @brief the kernel (H_W (c=0) or H_T (c=1)) */
  double M0_;/*!< @brief wilson mass (must be negative) */
  double mq_;/*!< @brief Bare quark mass \f$m_q\f$ */
  std::vector<double> omega_;/*!< @brief Weights defining the approximation */
  std::vector<double> bs_, cs_;
  std::vector<double> dp_, dm_;
  std::vector<double> es_, fs_;

  void set_arrays();

  Dirac_optimalDomainWall_params(XML::node DWF_node,DWFType Type);
  Dirac_optimalDomainWall_params(double b,double c,double M0,double mq,
				 const std::vector<double>& omega);
};

////////////////////////////////////////////////////////////////////////////
/*! @brief Defines the 5d Domain Wall operator
 *
 * Defines \f[D_{\rm dwf}(m_q) = \omega D_W(-m_0)(1+cL(m_q))+(1-L(m_q))\f]
 * with: \f[L(m) = P_+ L_+(m) + P_- L_-(m)\f]
 * using proj_p() e proj_m() methods
 *
 * Kernel is given by \f[H = \frac{\gamma_5 b D_W}{ 2 + c D_W } \f]
 */
class Dirac_optimalDomainWall : public DiracWilsonLike {
private:
  Dirac_optimalDomainWall_params Params_;
  size_t N5_;/*!< @brief Length of 5th dimension */
  double M0_;
  double mq_;

  const DiracWilsonLike* Dw_; /*!< @brief Dirac Kernel - any WilsonLike op */ 

  int Nvol_;
  ffmt_t ff_;  
  size_t f5size_;
  size_t f4size_;

  const Field get4d(const Field& f5,int s) const;
  void get4d(Field&, const Field& ,int ) const;
  void set5d(Field& f5,const Field& f4,int s) const;
  void add5d(Field& f5,const Field& f4,int s) const;
  void add5d(Field& f5,const Field& f4_1, const Field& f4_2,int s) const;

#ifndef IBM_BGQ_WILSON
  void get4d_c(Field&, const Field& ,const double&, int ) const;
  void set5d_c(Field& f5,const Field& f4,const double c,int s) const;  
  void mul5d(Field& f5,double fac,int s) const;
  void add5d_c(Field& f5,const Field& f4, double c,int s) const;
  void add5d_from5d(Field& f5,const Field& f,int s) const;
#endif
  void mult_offdiag(Field&,const Field&)const;/*! @brief it returns -kpp*D*f */
  void mult_full(Field&,const Field&)const;/*! @brief it returns (1-kpp*D)*f */

  void mult_dag_offdiag(Field&,const Field&)const; /*! @brief it returns -kpp*D^dag*f */
  void mult_dag_full(Field&,const Field&)const; /*! @brief it returns (1-kpp*D^dag)*f */

  void(Dirac_optimalDomainWall::*mult_core)(Field&,const Field&)const;
  void(Dirac_optimalDomainWall::*mult_dag_core)(Field&,const Field&)const;

  void proj_p(Field&,const Field&,int s=0)const;
  void proj_m(Field&,const Field&,int s=0)const;

public:
  /*! @brief constructors to create an instance with normal indexing */
  Dirac_optimalDomainWall(XML::node DWF_node,const DiracWilsonLike* Dw,
			  DWFType Type= Regular)
    :Params_(DWF_node,Type),
     N5_(Params_.N5_),M0_(Params_.M0_),mq_(Params_.mq_),
     Dw_(Dw),
     Nvol_(CommonPrms::instance()->Nvol()),
     ff_(Nvol_,N5_),
     f5size_(ff_.size()),
     f4size_(Dw->fsize()),
     mult_core(&Dirac_optimalDomainWall::mult_full),
     mult_dag_core(&Dirac_optimalDomainWall::mult_dag_full){
    assert(Dw_);
#if VERBOSITY>DEBUG_VERB_LEVEL
    CCIO::cout << "Created Dirac_optimalDomainWall" << std::endl;
#endif
  }

  Dirac_optimalDomainWall(double b,double c,double M0,double mq,
			  const std::vector<double>& omega,
			  const DiracWilsonLike* Dw)
    :Params_(b,c,M0,mq,omega),
     N5_(Params_.N5_),M0_(M0),mq_(mq),
     Dw_(Dw),
     Nvol_(CommonPrms::instance()->Nvol()),
     ff_(Nvol_,N5_),
     f5size_(ff_.size()),
     f4size_(Dw->fsize()),
     mult_core(&Dirac_optimalDomainWall::mult_full),
     mult_dag_core(&Dirac_optimalDomainWall::mult_dag_full){assert(Dw_);}
  
  /*! @brief copy constructor */
  Dirac_optimalDomainWall(const Dirac_optimalDomainWall& Dc, 
			  DWFType Type=Regular)
    :Params_(Dc.Params_),
     N5_(Params_.N5_),M0_(Params_.M0_),mq_(Params_.mq_),
     Dw_(Dc.Dw_),
     Nvol_(Dc.Nvol_),
     ff_(Dc.ff_),
     f5size_(Dc.f5size_),
     f4size_(Dc.f4size_),
     mult_core(&Dirac_optimalDomainWall::mult_full),
     mult_dag_core(&Dirac_optimalDomainWall::mult_dag_full){
    if(Type==PauliVillars) {
      mq_=1.0; 
      Params_.mq_=1.0;
    }
  }

  /*! @brief constructor for EvenOdd */
  Dirac_optimalDomainWall(XML::node DWF_node,const DiracWilsonLike* Dw,
			  DomainWallFermions::EvenOdd_tag,
			  DWFType Type= Regular)
    :Params_(DWF_node,Type),
     N5_(Params_.N5_),M0_(Params_.M0_),mq_(Params_.mq_),
     Dw_(Dw),
     Nvol_(CommonPrms::instance()->Nvol()/2),
     ff_(Nvol_,Params_.N5_),
     f5size_(ff_.size()),
     f4size_(Dw->fsize()),
     mult_core(&Dirac_optimalDomainWall::mult_offdiag),
     mult_dag_core(&Dirac_optimalDomainWall::mult_dag_offdiag){ assert(Dw_);}

  Dirac_optimalDomainWall(double b,double c,double M0,double mq,
			  const std::vector<double>& omega,
			  const DiracWilsonLike* Dw,
			  DomainWallFermions::EvenOdd_tag)
    :Params_(b,c,M0,mq,omega),
     N5_(Params_.N5_),M0_(M0),mq_(mq),
     Dw_(Dw),
     Nvol_(CommonPrms::instance()->Nvol()/2),
     ff_(Nvol_,Params_.N5_),
     f5size_(ff_.size()),
     f4size_(Dw->fsize()),
     mult_core(&Dirac_optimalDomainWall::mult_offdiag),
     mult_dag_core(&Dirac_optimalDomainWall::mult_dag_offdiag){assert(Dw_);}


  size_t f4size() const{ return f4size_;}
  size_t fsize() const{ return f5size_;}
  size_t gsize()const{return Dw_->gsize();}
  double getMass() const{return Params_.mq_;}

  const Field* getGaugeField_ptr()const{ return Dw_->getGaugeField_ptr(); }

  const Field mult(const Field&)const;
  const Field mult_dag(const Field&)const;

  // mult in the heavy quark limit
  const Field mult_hop5(const Field& f5)const;    /*! @brief mult in the heavy M0 limit*/
  const Field mult_hop5_inv(const Field& f5)const;/*! @brief mult_inv in the heavy M0 limit*/
  const Field mult_hop5_dag(const Field& f5)const;/*! @brief mult_dag in the heavy M0 limit*/
  const Field mult_hop5_dinv(const Field& f5)const;/*! @brief mult in the heavy M0 limit*/

  const Field Dminus(const Field&)const;
  const Field gamma5(const Field&)const;
  //const Field Bproj(const Field& v5d)const;
  //const Field Bproj_dag(const Field& v4d)const;
  const Field R5(const Field&)const;
  const Field R5g5(const Field&)const;

  void gamma5_4d(Field&,const Field&)const;
  void md_force_p(Field&,const Field&,const Field&)const;
  void md_force_m(Field&,const Field&,const Field&)const;
  const Field md_force(const Field& eta,const Field& zeta)const;
  void update_internal_state(){}

  // BGQ optimizations
#ifdef IBM_BGQ_WILSON
  void mult_hop(Field&,const Field&)const;
  void mult_hop_omp(Field&,const void*)const;
  void mult_hop_omp_allocated(Field&,const void*,void*,void*,int,int)const;
  void mult_hop_dag(Field&,const Field&)const;
  void mult_hop_dag_omp(Field&,const void*)const;
  void mult_hop_dag_omp_allocated(Field&,const void*,void*,void*,int,int)const;

  typedef std::vector<Field> prop_t;

  void solve_eo_5d(Field&,const Field&,SolverOutput&,int,double)const;

  void Dirac_optimalDomainWall::solve_ms_init(std::vector<Field>&,
					      std::vector<Field>&,
					      Field&, Field&,
					      double& rr,
					      std::vector<double>&,
					      std::vector<double>&,
					      std::vector<double>&,
					      double&,
					      double&) const;
  void Dirac_optimalDomainWall::solve_ms_eo_5d(prop_t&, 
					       const Field&,
					       SolverOutput&, 
					       const std::vector<double>&, 
					       int, double) const;
#endif
};


#endif
