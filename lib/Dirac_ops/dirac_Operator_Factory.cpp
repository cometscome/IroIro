/*! @file dirac_Operator_Factory.cpp
 *  @brief Implementation of the FactoryCreator for Dirac operators
 * Time-stamp: <2013-04-17 15:58:45 noaki>
 */
#include "dirac_Operator_Factory.hpp"
#include "Communicator/comm_io.hpp"
#include <string.h>

/// Dirac_Wilson
Dirac_Wilson* DiracWilsonFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_Wilson(Dirac_node_,Gfield);}

/// Dirac_Wilson_EvenOdd
Dirac_Wilson_EvenOdd* DiracWilsonEvenOddFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_Wilson_EvenOdd(Dirac_node_,Gfield);}

/// Dirac_Wilson_Brillouin
Dirac_Wilson_Brillouin* DiracWilsonBrillouinFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_Wilson_Brillouin(Dirac_node_,Gfield);}

/// Dirac_Wilson_Brillouin_Imp
Dirac_Wilson_Brillouin_Imp*  DiracWilsonBrillouinImpFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_Wilson_Brillouin_Imp(Dirac_node_,Gfield); }

/// Dirac_Clover
Dirac_Clover* DiracCloverFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_Clover(Dirac_node_,Gfield); }

/// Dirac_optimalDomainWall
Dirac_optimalDomainWall* DiracDWF5dFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_optimalDomainWall(Dirac_node_,KernelFactory_.get()->getDiracOperatorWL(Gfield),Gfield); }

Dirac_optimalDomainWall* DiracDWF5dFactory::
getDiracOperatorPV(Field* const Gfield){
  return new Dirac_optimalDomainWall(Dirac_node_,KernelFactory_.get()->getDiracOperatorWL(Gfield),Gfield,PauliVillars); }

/// Dirac_optimalDomainWall_EvenOdd
Dirac_optimalDomainWall_EvenOdd* DiracDWF5dEvenOddFactory::
getDiracOperatorWL(Field* const Gfield){
  return new Dirac_optimalDomainWall_EvenOdd(Dirac_node_,Gfield); }

Dirac_optimalDomainWall_EvenOdd* DiracDWF5dEvenOddFactory::
getDiracOperatorPV(Field* const Gfield){
  return new Dirac_optimalDomainWall_EvenOdd(Dirac_node_,Gfield,PauliVillars);}

/// Dirac_DWF4DfullSolv
DiracWilsonLike* DiracDWF4DfullFactory::
getDiracOperatorWL(Field* const Gfield){
  return getDiracOperator4D(Gfield); }

Dirac_optimalDomainWall_4D* DiracDWF4DfullFactory::
getDiracOperator4D(Field* const Gfield){

  DW5D_.save(DiracFactory_.get()->getDiracOperatorWL(Gfield));
  Fopr_.save(new Fopr_DdagD_Precondition(DW5D_.get()));
  Solver_.save(SolverFactory_.get()->getSolver(Fopr_.get()));

  DW5D_PV_.save(DiracFactory_.get()->getDiracOperatorPV(Gfield));
  Fopr_PV_.save(new Fopr_DdagD_Precondition(DW5D_PV_.get()));
  Solver_PV_.save(SolverFactory_.get()->getSolver(Fopr_PV_.get()));

  return new Dirac_optimalDomainWall_4D_fullSolv(DW5D_.get(),
						 DW5D_PV_.get(),
						 Solver_.get(),
						 Solver_PV_.get());
}

/// Dirac_DWF4DeoSolv
DiracWilsonLike* DiracDWF4DeoFactory::
getDiracOperatorWL(Field* const Gfield){
  return getDiracOperator4D(Gfield); }

Dirac_optimalDomainWall_4D* DiracDWF4DeoFactory::
getDiracOperator4D(Field* const Gfield){
  DW5D_.save(   DiracFactory_.get()->getDiracOperatorWL(Gfield));
  DW5D_EO_.save(DiracEOFactory_.get()->getDiracOperatorWL(Gfield));
  FoprEO_.save(new Fopr_DdagD(DW5D_EO_.get()));
  SolverEO_.save(SolverFactory_.get()->getSolver(FoprEO_.get()));
  Inv_.save(new EvenOddUtils::Inverter_WilsonLike(DW5D_EO_.get(),
						  SolverEO_.get()));

  DW5DPV_.save(    DiracFactory_.get()->getDiracOperatorPV(Gfield));
  DW5D_EO_PV_.save(DiracEOFactory_.get()->getDiracOperatorPV(Gfield));
  FoprEO_PV_.save(new Fopr_DdagD(DW5D_EO_PV_.get()));
  SolverEO_PV_.save(SolverFactory_.get()->getSolver(FoprEO_PV_.get()));
  Inv_PV_.save(new EvenOddUtils::Inverter_WilsonLike(DW5D_EO_PV_.get(),
						     SolverEO_PV_.get()));
  
  return new Dirac_optimalDomainWall_4D_eoSolv(DW5D_.get(),DW5DPV_.get(),
					       Inv_.get(),Inv_PV_.get());
}

/// Dirac_staggered_EvenOdd
Dirac_staggered_EvenOdd* DiracStaggeredEvenOddFactory::
getDiracOperatorSTG(Dstagg::Dtype dt,Field* const Gfield){
  return new Dirac_staggered_EvenOdd(Dirac_node_,dt,Gfield); }

/// Dirac_staggered_EvenOdd_Adjoint
#if NC_==3
Dirac_staggered_EvenOdd_Adjoint* DiracStaggeredEvenOddAdjointFactory::
getDiracOperatorSTG(Dstagg::Dtype dt,Field* const Gfield){
  return new Dirac_staggered_EvenOdd_Adjoint(Dirac_node_,dt,Gfield); }
#endif
