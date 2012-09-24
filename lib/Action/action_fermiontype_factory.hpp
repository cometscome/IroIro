/*!
 * @file action_fermiontype_factory.hpp 
 * @brief Declaration of FermionType action factories
 */
#ifndef ACTION_FERMION_FACT_
#define ACTION_FERMION_FACT_

#include "include/pugi_interface.h"
#include "Tools/RAIIFactory.hpp"

#include "action_fermiontype_factory_abs.hpp"
#include "Action/action_Nf2.hpp"
#include "Action/action_Nf.hpp"
#include "Action/action_Nf2_ratio.hpp"
#include "Action/action_Nf_ratio.hpp"
#include "Action/action_Nf2_DomainWall.hpp"
#include "Action/action_staggered.hpp"
#include "Action/action_staggered_ratio.hpp"
#include "Solver/solver_CG.hpp"
#include "Dirac_ops/dirac_Operator_Factory.hpp"
#include "Solver/solver_Factory.hpp"

///////////////////////////////////////////////////////////////////////
class TwoFlavorActionFactory : public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracWilsonLike> Kernel;
  RaiiFactoryObj<Fopr_DdagD> HermitianOp;
  RaiiFactoryObj<Solver> Solv;
 
  const XML::node Action_node;
  bool smearing;
public:
  TwoFlavorActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node, "smeared", smearing);

    XML::descend(node,"Kernel",MANDATORY);
    DiracObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));
    XML::next_sibling(node,"Solver", MANDATORY);
    SolverObj.save(SolverOperators::createSolverOperatorFactory(node));
  }

  ~TwoFlavorActionFactory(){}
private:
  Action_Nf2* getFermionAction(GaugeField* const F, SmartConf* const SC){
    // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    Kernel.save(DiracObj.get()->getDiracOperatorWL(&(Links->data)));
    HermitianOp.save(new Fopr_DdagD(Kernel.get()));
    Solv.save(SolverObj.get()->getSolver(HermitianOp.get()));

    return new Action_Nf2(Links, Kernel.get(), Solv.get(), smearing, SC);
  }
};

//////////////////////////////////////////////////////////////////
class NfFlavorsActionFactory : public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracObj;
  RaiiFactoryObj<RationalSolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracWilsonLike> Kernel;
  RaiiFactoryObj<Fopr_DdagD> HermitianOp;
  RaiiFactoryObj<RationalSolver> Solv;

  const XML::node Action_node;
  bool smearing;
public:
  NfFlavorsActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Kernel",MANDATORY);
    DiracObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));  
    XML::next_sibling(node,"RationalSolver",MANDATORY);
    SolverObj.save(SolverOperators::createRationalSolverOperatorFactory(node));
  }

  ~NfFlavorsActionFactory(){}
private: 
  Action_Nf* getFermionAction(GaugeField* const F,SmartConf* const SC) {
   // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);
    
    Kernel.save(DiracObj.get()->getDiracOperatorWL(&(Links->data)));
    HermitianOp.save(new Fopr_DdagD(Kernel.get()));
    Solv.save(SolverObj.get()->getSolver(HermitianOp.get()));
    return new Action_Nf(Links,Kernel.get(),Solv.get(), 
			 Action_Nf_params(Action_node),smearing,SC);
  }
};

////////////////////////////////////////////////////
class TwoFlavorRatioActionFactory :public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracNumObj;
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracDenomObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverNumObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverDenomObj;

  RaiiFactoryObj<DiracWilsonLike> DiracNumerator;
  RaiiFactoryObj<DiracWilsonLike> DiracDenominator;
  RaiiFactoryObj<Solver> Solver1;
  RaiiFactoryObj<Solver> Solver2;

  const XML::node Action_node;
  bool smearing;
public:
  ~TwoFlavorRatioActionFactory(){}

  TwoFlavorRatioActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Numerator",MANDATORY);
    DiracNumObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node)); 
    XML::next_sibling(node,"Denominator",MANDATORY);
    DiracDenomObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));
    XML::next_sibling(node,"SolverNumerator",MANDATORY);
    SolverNumObj.save(SolverOperators::createSolverOperatorFactory(node));
    XML::next_sibling(node,"SolverDenominator",MANDATORY);
    SolverDenomObj.save(SolverOperators::createSolverOperatorFactory(node));
  }
private:  
  Action_Nf2_ratio* getFermionAction(GaugeField* const F, SmartConf* const SC){
   // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DiracNumerator.save(DiracNumObj.get()->getDiracOperatorWL(&(Links->data)));
    DiracDenominator.save(DiracDenomObj.get()->getDiracOperatorWL(&(Links->data)));
    
    Solver1.save(SolverNumObj.get()->getSolver(new Fopr_DdagD(DiracNumerator.get())));
    Solver2.save(SolverDenomObj.get()->getSolver(new Fopr_DdagD(DiracDenominator.get())));

    return new Action_Nf2_ratio(Links,
				DiracNumerator.get(),DiracDenominator.get(),
				Solver1.get(),Solver2.get(),
				smearing, SC); 
  }
};

////////////////////////////////////////////////////
class NfFlavorRatioActionFactory : public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracNumObj;
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracDenomObj;
  RaiiFactoryObj<RationalSolverOperatorFactory> SolverNumObj;
  RaiiFactoryObj<RationalSolverOperatorFactory> SolverDenomObj;

  RaiiFactoryObj<DiracWilsonLike> DiracNumerator;
  RaiiFactoryObj<DiracWilsonLike> DiracDenominator;
  RaiiFactoryObj<RationalSolver> Solver1;
  RaiiFactoryObj<RationalSolver> Solver2;

  const XML::node Action_node;
  bool smearing;
public:
  ~NfFlavorRatioActionFactory(){}

  NfFlavorRatioActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Numerator",MANDATORY);
    DiracNumObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node)); 
    XML::next_sibling(node,"Denominator",MANDATORY);
    DiracDenomObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));
    XML::next_sibling(node,"RationalSolverNumerator",MANDATORY);
    SolverNumObj.save(SolverOperators::createRationalSolverOperatorFactory(node));
    XML::next_sibling(node,"RationalSolverDenominator",MANDATORY);
    SolverDenomObj.save(SolverOperators::createRationalSolverOperatorFactory(node));
  }
private:  
  Action_Nf_ratio* getFermionAction(GaugeField* const F,SmartConf* const SC){
   // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DiracNumerator.save(DiracNumObj.get()->getDiracOperatorWL(&(Links->data)));
    DiracDenominator.save(DiracDenomObj.get()->getDiracOperatorWL(&(Links->data)));
    
    Solver1.save(SolverNumObj.get()->getSolver(new Fopr_DdagD(DiracNumerator.get())));
    Solver2.save(SolverDenomObj.get()->getSolver(new Fopr_DdagD(DiracDenominator.get())));

    return new Action_Nf_ratio(Links,
			       DiracNumerator.get(),DiracDenominator.get(),
			       Solver1.get(),Solver2.get(),
			       Action_Nf_ratio_params(Action_node),
			       smearing, SC);  
  }
};

////////////////////////////////////////////////////
class TwoFlavorDomainWall5dActionFactory :public FermionActionFactory {

  RaiiFactoryObj<DiracDWF5dOperatorFactory> DiracObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracWilsonLike> DWF5d_Kernel;
  RaiiFactoryObj<DiracWilsonLike> DWF5d_KernelPV;
  RaiiFactoryObj<Fopr_DdagD_Precondition> HermitianOp;
  RaiiFactoryObj<Fopr_DdagD_Precondition> HermitianOpPV;
  RaiiFactoryObj<Solver> Solv;
  RaiiFactoryObj<Solver> SolvPV;
  
  const XML::node Action_node;
  bool smearing;
public:
  ~TwoFlavorDomainWall5dActionFactory(){}

  TwoFlavorDomainWall5dActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Kernel5D",MANDATORY);
    DiracObj.save(DiracOperators::createDiracDWF5dOperatorFactory(node));
    XML::next_sibling(node,"Solver",MANDATORY);
    SolverObj.save(SolverOperators::createSolverOperatorFactory(node));
  }
private:  
  Action_Nf2_ratio* getFermionAction(GaugeField* const F,SmartConf* const SC){
    // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DWF5d_Kernel.save(  DiracObj.get()->getDiracOperatorWL(&(Links->data)));
    DWF5d_KernelPV.save(DiracObj.get()->getDiracOperatorPV(&(Links->data)));

    HermitianOp.save(  new Fopr_DdagD_Precondition(DWF5d_Kernel.get()));
    HermitianOpPV.save(new Fopr_DdagD_Precondition(DWF5d_KernelPV.get()));
    Solv.save(  SolverObj.get()->getSolver(HermitianOp.get()));
    SolvPV.save(SolverObj.get()->getSolver(HermitianOpPV.get()));
    return new Action_Nf2_ratio(Links,
				DWF5d_Kernel.get(),DWF5d_KernelPV.get(),
				Solv.get(),SolvPV.get(),
				smearing, SC);
  }
};

////////////////////////////////////////////////////
#ifdef IBM_BGQ_WILSON

class TwoFlavorDomainWall5dEO_BGQ_ActionFactory : public FermionActionFactory {

  RaiiFactoryObj<DiracDWF5dEvenOddFactory> DiracObj;
  RaiiFactoryObj<SolverCG_DWF_opt_Factory> SolverObj;

  RaiiFactoryObj<Dirac_optimalDomainWall_EvenOdd> DWF5d_Kernel;
  RaiiFactoryObj<Dirac_optimalDomainWall_EvenOdd> DWF5d_KernelPV;
  RaiiFactoryObj<Solver> Solv;
  RaiiFactoryObj<Solver> SolvPV;
  
  const XML::node Action_node;
  bool smearing;
public:
  ~TwoFlavorDomainWall5dEO_BGQ_ActionFactory(){}

  TwoFlavorDomainWall5dEO_BGQ_ActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node, "smeared", smearing);

    XML::descend(node,"Kernel5D", MANDATORY);
    DiracObj.save(new DiracDWF5dEvenOddFactory(node));
    XML::next_sibling(node,"Solver_DWF-EO_BGQ", MANDATORY);
    SolverObj.save(new SolverCG_DWF_opt_Factory(node));
  }
private:  
  Action_Nf2_ratio* getFermionAction(GaugeField* const F, SmartConf* const SC){
    // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DWF5d_Kernel.save(  DiracObj.get()->getDiracOperatorWL(&(Links->data)));
    DWF5d_KernelPV.save(DiracObj.get()->getDiracOperatorPV(&(Links->data)));

    Solv.save(  SolverObj.get()->getSolver(DWF5d_Kernel.get()));
    SolvPV.save(SolverObj.get()->getSolver(DWF5d_KernelPV.get()));

    return new Action_Nf2_ratio(Links,
				DWF5d_Kernel.get(),DWF5d_KernelPV.get(),
				Solv.get(),SolvPV.get(),
				smearing, SC);
  }
};

////////////////////////////////////////////////////
class TwoFlavorRatioDomainWall5dEO_BGQ_ActionFactory : public FermionActionFactory {

  RaiiFactoryObj<DiracDWF5dEvenOddFactory> DiracNumObj;
  RaiiFactoryObj<DiracDWF5dEvenOddFactory> DiracDenomObj;
  RaiiFactoryObj<SolverCG_DWF_opt_Factory> SolverNumObj;
  RaiiFactoryObj<SolverCG_DWF_opt_Factory> SolverDenomObj;

  RaiiFactoryObj<Dirac_optimalDomainWall_EvenOdd> DiracNumerator;
  RaiiFactoryObj<Dirac_optimalDomainWall_EvenOdd> DiracDenominator;
  RaiiFactoryObj<Solver> Solver1;
  RaiiFactoryObj<Solver> Solver2;

  const XML::node Action_node;
  bool smearing;
public:
  ~TwoFlavorRatioDomainWall5dEO_BGQ_ActionFactory(){}

  TwoFlavorRatioDomainWall5dEO_BGQ_ActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Numerator", MANDATORY);
    DiracNumObj.save(new DiracDWF5dEvenOddFactory(node)); 
    XML::next_sibling(node,"Denominator", MANDATORY);
    DiracDenomObj.save(new DiracDWF5dEvenOddFactory(node));
    XML::next_sibling(node,"SolverNumerator", MANDATORY);
    SolverNumObj.save(new SolverCG_DWF_opt_Factory(node));
    XML::next_sibling(node,"SolverDenominator", MANDATORY);
    SolverDenomObj.save(new SolverCG_DWF_opt_Factory(node));
  }
private:  
  Action_Nf2_ratio* getFermionAction(GaugeField* const F, SmartConf* const SC){
   // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DiracNumerator.save(DiracNumObj.get()->getDiracOperatorWL(&(Links->data)));
    DiracDenominator.save(DiracDenomObj.get()->getDiracOperatorWL(&(Links->data)));
    
    Solver1.save(SolverNumObj.get()->getSolver(DiracNumerator.get()));
    Solver2.save(SolverDenomObj.get()->getSolver(DiracDenominator.get()));

    return new Action_Nf2_ratio(Links,
				DiracNumerator.get(),DiracDenominator.get(),
				Solver1.get(),Solver2.get(),
				smearing, SC); 
  }
};
#endif

////////////////////////////////////////////////////
class NfFlavorDomainWall5dActionFactory : public FermionActionFactory {

  RaiiFactoryObj<DiracDWF5dOperatorFactory> DiracObj;
  RaiiFactoryObj<RationalSolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracWilsonLike> DWF5d_Kernel;
  RaiiFactoryObj<DiracWilsonLike> DWF5d_KernelPV;
  RaiiFactoryObj<Fopr_DdagD_Precondition> HermitianOp;
  RaiiFactoryObj<Fopr_DdagD_Precondition> HermitianOpPV;
  RaiiFactoryObj<RationalSolver> Solv;
  RaiiFactoryObj<RationalSolver> SolvPV;
  
  const XML::node Action_node;
  bool smearing;
public:
  ~NfFlavorDomainWall5dActionFactory(){}

  NfFlavorDomainWall5dActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Kernel5D",MANDATORY);
    DiracObj.save(DiracOperators::createDiracDWF5dOperatorFactory(node));
    XML::next_sibling(node,"RationalSolver",MANDATORY);
    SolverObj.save(SolverOperators::createRationalSolverOperatorFactory(node));
  }
private:  
  Action_Nf_ratio* getFermionAction(GaugeField* const F,SmartConf* const SC){
    // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DWF5d_Kernel.save(  DiracObj.get()->getDiracOperatorWL(&(Links->data)));
    DWF5d_KernelPV.save(DiracObj.get()->getDiracOperatorPV(&(Links->data)));

    HermitianOp.save(  new Fopr_DdagD_Precondition(DWF5d_Kernel.get()));
    HermitianOpPV.save(new Fopr_DdagD_Precondition(DWF5d_KernelPV.get()));
    Solv.save(  SolverObj.get()->getSolver(HermitianOp.get()));
    SolvPV.save(SolverObj.get()->getSolver(HermitianOpPV.get()));
    return new Action_Nf_ratio(Links,
			       DWF5d_Kernel.get(),DWF5d_KernelPV.get(),
			       Solv.get(),SolvPV.get(),
			       Action_Nf_ratio_params(Action_node),
			       smearing,SC);
  }
};

class FourFlavorStaggeredActionFactory :public FermionActionFactory{
  RaiiFactoryObj<DiracStaggeredEvenOddLikeOperatorFactory> DiracObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracStaggeredEvenOddLike> Kernel;
  RaiiFactoryObj<Fopr_HD> HermitianOp;
  RaiiFactoryObj<Solver> Solv;
 
  const XML::node Action_node;
  bool smearing;
public:
  FourFlavorStaggeredActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Kernel",MANDATORY);
    DiracObj.save(DiracOperators::createDiracStaggeredEvenOddLikeOperatorFactory(node));
    XML::next_sibling(node,"Solver",MANDATORY);
    SolverObj.save(SolverOperators::createSolverOperatorFactory(node));
  }
  
  ~FourFlavorStaggeredActionFactory(){}
private:
  Action_staggered* getFermionAction(GaugeField* const F,SmartConf* const SC){
    // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    Kernel.save(DiracObj.get()->getDiracOperatorSTG(&(Links->data)));
    HermitianOp.save(new Fopr_HD(Kernel.get()));
    Solv.save(SolverObj.get()->getSolver(HermitianOp.get()));

    return new Action_staggered(Links,Kernel.get(),Solv.get(),smearing,SC);
  }
};

class FourFlavorStaggeredRatioActionFactory :public FermionActionFactory{
  RaiiFactoryObj<DiracStaggeredEvenOddLikeOperatorFactory> DiracNumObj;
  RaiiFactoryObj<DiracStaggeredEvenOddLikeOperatorFactory> DiracDenomObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverNumObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverDenomObj;

  RaiiFactoryObj<DiracStaggeredEvenOddLike> DiracNumerator;
  RaiiFactoryObj<DiracStaggeredEvenOddLike> DiracDenominator;
  RaiiFactoryObj<Solver> Solver1;
  RaiiFactoryObj<Solver> Solver2;

  const XML::node Action_node;
  bool smearing;
public:
  FourFlavorStaggeredRatioActionFactory(XML::node node)
    :Action_node(node),smearing(false){
    XML::read(node,"smeared",smearing);

    XML::descend(node,"Numerator",MANDATORY);
    DiracNumObj.save(DiracOperators::createDiracStaggeredEvenOddLikeOperatorFactory(node)); 
    XML::next_sibling(node,"Denominator",MANDATORY);
    DiracDenomObj.save(DiracOperators::createDiracStaggeredEvenOddLikeOperatorFactory(node));
    XML::next_sibling(node,"SolverNumerator",MANDATORY);
    SolverNumObj.save(SolverOperators::createSolverOperatorFactory(node));
    XML::next_sibling(node,"SolverDenominator",MANDATORY);
    SolverDenomObj.save(SolverOperators::createSolverOperatorFactory(node));
  }

  ~FourFlavorStaggeredRatioActionFactory(){}
private:  
  Action_staggered_ratio* getFermionAction(GaugeField* const F,SmartConf* const SC){
    // select links according to smearing
    GaugeField* Links = SC->select_conf(smearing);

    DiracNumerator.save(DiracNumObj.get()->getDiracOperatorSTG(&(Links->data)));
    DiracDenominator.save(DiracDenomObj.get()->getDiracOperatorSTG(&(Links->data)));
    
    Solver1.save(SolverNumObj.get()->getSolver(new Fopr_HD(DiracNumerator.get())));
    Solver2.save(SolverDenomObj.get()->getSolver(new Fopr_HD(DiracDenominator.get())));

    return new Action_staggered_ratio(Links,
				      DiracNumerator.get(),DiracDenominator.get(),
				      Solver1.get(),Solver2.get(),
				      smearing, SC); 
  }
};

//Add new factories here
//....

#endif
