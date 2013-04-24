/*!
 * @file dirac_DomainWall_EvenOdd.hpp
 * @brief Declaration of class Dirac_optimalDomainWall_EvenOdd (5d operator)
 Time-stamp: <2013-04-23 16:09:19 noaki>
 */
#ifndef DIRAC_OPTIMALDOMAINWALL_EVENODD_INCLUDED
#define DIRAC_OPTIMALDOMAINWALL_EVENODD_INCLUDED

#include "dirac_DomainWall.hpp"

#ifdef IBM_BGQ_WILSON
struct SolverOutput;
#endif

static  double mult_timer;
static  double multdag_timer;
/*!
 * @brief Defines the 5d Domain Wall operator with even/odd site indexing
 */

class Dirac_optimalDomainWall_EvenOdd : public DiracWilsonLike_EvenOdd {

  const Dirac_optimalDomainWall Deo_;
  const Dirac_optimalDomainWall Doe_;

  void md_force_eo(Field&,const Field&,const Field&)const;
  void md_force_oe(Field&,const Field&,const Field&)const;
  
  Dirac_optimalDomainWall_EvenOdd(const Dirac_optimalDomainWall_EvenOdd&);
  /*!< simple copy is prohibited */

public:
  Dirac_optimalDomainWall_EvenOdd(XML::node DWF_node,
				  DiracWilsonLike_EvenOdd* Kernel,
				  const Field* u,
				  DWFType Type=Regular)
    :Deo_(DWF_node,Kernel->getDeo(),u,DomainWallFermions::EvenOdd_tag(),Type),
     Doe_(DWF_node,Kernel->getDoe(),u,DomainWallFermions::EvenOdd_tag(),Type){
    //
#if VERBOSITY>4
    CCIO::cout<<"Dirac_optimalDomainWall_Evenodd created"<<std::endl;
#endif
  }

  /*! @brief copy constractor to create Pauli-Villars operator */
  Dirac_optimalDomainWall_EvenOdd(const Dirac_optimalDomainWall_EvenOdd& D, 
				  DWFType Type=Regular)
    :Deo_(D.Deo_,Type),Doe_(D.Doe_,Type){}
  
  Dirac_optimalDomainWall_EvenOdd(double b,double c,double M0,double mq,
				  const std::vector<double>& omega,
				  const DiracWilsonLike* Keo,
				  const DiracWilsonLike* Koe,
				  const Field* u)
    :Deo_(b,c,M0,mq,omega,Keo,u,DomainWallFermions::EvenOdd_tag()),
     Doe_(b,c,M0,mq,omega,Koe,u,DomainWallFermions::EvenOdd_tag()){}
  
  ~Dirac_optimalDomainWall_EvenOdd(){
    CCIO::cout << "DWF Timer mult: "<< mult_timer << "\n";
    CCIO::cout << "DWF Timer multdag: "<< multdag_timer << "\n";
  }
  
  size_t f4size()const{ return Deo_.f4size();}
  size_t fsize()const{ return Deo_.fsize();}
  size_t gsize()const{ return Deo_.gsize();}

  const DiracWilsonLike* getDeo()const{ return &Deo_;}
  const DiracWilsonLike* getDoe()const{ return &Doe_;}
  
  double getMass() const{return Deo_.getMass();}
  const Field mult(const Field&)const;
  const Field mult_dag(const Field&)const;

  //Preconditioning methods
  const Field mult_prec    (const Field& f)const{return f;}//empty now
  const Field mult_dag_prec(const Field& f)const{return f;}//empty now
  const Field left_prec     (const Field& f)const{return f;}//empty now
  const Field left_dag_prec (const Field& f)const{return f;}//empty now
  const Field right_prec    (const Field& f)const{return f;}//empty now
  const Field right_dag_prec(const Field& f)const{return f;}//empty now
  //////////////////////////////////////////////////////////////////////

  const Field md_force( const Field& eta,const Field& zeta) const;
  ////
  const Field mult_hop5_inv(const Field& f5) const;

  const Field gamma5(const Field& f5) const{ return Deo_.gamma5(f5);}
  ////
  const Field mult_eo(const Field& f) const; 
  const Field mult_oe(const Field& f) const; 

  const Field mult_eo_dag(const Field& f) const;
  const Field mult_oe_dag(const Field& f) const;

  const Field mult_oo(const Field& f)const;
  const Field mult_oo_inv(const Field& f)const;
  const Field mult_oo_dinv(const Field& f)const;

  const Field mult_ee(const Field& f)const;
  const Field mult_ee_inv(const Field& f)const;
  const Field mult_ee_dinv(const Field& f)const;

  void update_internal_state(){} 

  ////////////////////////////
#ifdef IBM_BGQ_WILSON

 typedef std::vector<Field> prop_t;
  void solve_eo(Field&, const Field&, SolverOutput&, int, double) const;
  void solve_ms_eo(prop_t& , const Field& , SolverOutput&, 
		   const std::vector<double>&, int, double)const;
#endif
};


#endif
