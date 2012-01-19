/*!
 * @file action_fermiontype_factory.hpp 
 *
 * @brief Declaration of FermionType action factories
 */

#ifndef ACTION_FERMION_FACT_
#define ACTION_FERMION_FACT_

#include "include/pugi_interface.h"
#include "Tools/RAIIFactory.hpp"

#include "action_Factory.hpp"
#include "Action/action_Nf2.hpp"
#include "Action/action_Nf2_ratio.hpp"
#include "Action/action_Nf2_DomainWall.hpp"
#include "Solver/solver_CG.h"
#include "Dirac_ops/dirac_Operator_Factory.hpp"
#include "Solver/solver_Factory.hpp"

class FermionActionFactory : public ActionFactory {
  virtual Action* getFermionAction(const Format::Format_G&,
				   Field* const) = 0;
public:
  Action* getAction(const Format::Format_G& GaugeForm,
		    Field* const GaugeField) {
    return getFermionAction(GaugeForm,GaugeField);
  }
};

///////////////////////////////////////////////////////////////////////

class TwoFlavorActionFactory : public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracWilsonLike> Kernel;
  RaiiFactoryObj<Fopr_DdagD> HermitianOp;
  RaiiFactoryObj<Solver> Solv;
 
  const XML::node Action_node;

public:
  TwoFlavorActionFactory(XML::node node):Action_node(node){
    XML::descend(node,"Kernel",MANDATORY);
    DiracObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));
    XML::next_sibling(node,"Solver", MANDATORY);
    SolverObj.save(SolverOperators::createSolverOperatorFactory(node));
  }

  ~TwoFlavorActionFactory(){}
private:
  Action_Nf2* getFermionAction(const Format::Format_G& Form,
			       Field* const GaugeField){
    Kernel.save(DiracObj.get()->getDiracOperator(GaugeField));
    HermitianOp.save(new Fopr_DdagD(Kernel.get()));
    Solv.save(SolverObj.get()->getSolver(HermitianOp.get()));
    return new Action_Nf2(GaugeField, Kernel.get(), Solv.get());
  }
};

////////////////////////////////////////////////////

class TwoFlavorRatioActionFactory : public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracNumObj;
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracDenomObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverNumObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverDenomObj;

  RaiiFactoryObj<DiracWilsonLike> DiracNumerator;
  RaiiFactoryObj<DiracWilsonLike> DiracDenominator;
  RaiiFactoryObj<Solver> Solver1;
  RaiiFactoryObj<Solver> Solver2;

  const XML::node Action_node;

public:
  ~TwoFlavorRatioActionFactory(){}

  TwoFlavorRatioActionFactory(XML::node node):Action_node(node){
    XML::descend(node,"Numerator", MANDATORY);
    DiracNumObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node)); 
    XML::next_sibling(node,"Denominator", MANDATORY);
    DiracDenomObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));
    XML::next_sibling(node,"SolverNumerator", MANDATORY);
    SolverNumObj.save(SolverOperators::createSolverOperatorFactory(node));
    XML::next_sibling(node,"SolverDenominator", MANDATORY);
    SolverDenomObj.save(SolverOperators::createSolverOperatorFactory(node));
  }
  
private:  
  Action_Nf2_ratio* getFermionAction(const Format::Format_G& Form,
				     Field* const GaugeField){
    DiracNumerator.save(DiracNumObj.get()->getDiracOperator(GaugeField));
    DiracDenominator.save(DiracDenomObj.get()->getDiracOperator(GaugeField));
    
    Solver1.save(SolverNumObj.get()->getSolver(new Fopr_DdagD(DiracNumerator.get())));
    Solver2.save(SolverDenomObj.get()->getSolver(new Fopr_DdagD(DiracDenominator.get())));

    return new Action_Nf2_ratio(GaugeField,
				DiracNumerator.get(),
				DiracDenominator.get(),
				Solver1.get(),
				Solver2.get()); 
  }
};
////////////////////////////////////////////////////

class TwoFlavorDomainWallActionFactory : public FermionActionFactory {

  RaiiFactoryObj<DiracDWF5dFactory> DiracObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverObj;

  RaiiFactoryObj<Dirac_optimalDomainWall> DWF5d_Kernel;
  RaiiFactoryObj<Dirac_optimalDomainWall> DWF5d_KernelPV;
  RaiiFactoryObj<Fopr_DdagD_Precondition> HermitianOp;
  RaiiFactoryObj<Fopr_DdagD_Precondition> HermitianOpPV;
  RaiiFactoryObj<Solver> Solv;
  RaiiFactoryObj<Solver> SolvPV;
  
  const XML::node Action_node;

public:
  ~TwoFlavorDomainWallActionFactory(){}

  TwoFlavorDomainWallActionFactory(XML::node node):Action_node(node){
    CCIO::cout<<"TwoFlavorDomainWallActionFactory called"<<std::endl;
    XML::descend(node,"Kernel5D", MANDATORY);
    DiracObj.save(new DiracDWF5dFactory(node));
    XML::next_sibling(node,"Solver", MANDATORY);
    SolverObj.save(SolverOperators::createSolverOperatorFactory(node));
  }

private:  
  Action_Nf2_ratio* getFermionAction(const Format::Format_G& Form,
				     Field* const GaugeField){
    DWF5d_Kernel.save(DiracObj.get()->getDiracOperator(GaugeField));
    DWF5d_KernelPV.save(new Dirac_optimalDomainWall(*DWF5d_Kernel.get(),
						    PauliVillars));
    HermitianOp.save(new Fopr_DdagD_Precondition(DWF5d_Kernel.get()));
    HermitianOpPV.save(new Fopr_DdagD_Precondition(DWF5d_KernelPV.get()));
    Solv.save(SolverObj.get()->getSolver(HermitianOp.get()));
    SolvPV.save(SolverObj.get()->getSolver(HermitianOpPV.get()));
    return new Action_Nf2_ratio(GaugeField,
				DWF5d_Kernel.get(),
				DWF5d_KernelPV.get(),
				Solv.get(),
				SolvPV.get());
  }
};
///////////////////////////////////////////////////////////////////////
/*
class TwoFlavorEvenOddActionFactory : public FermionActionFactory {
  RaiiFactoryObj<DiracWilsonLikeOperatorFactory> DiracObj;
  RaiiFactoryObj<SolverOperatorFactory> SolverObj;

  RaiiFactoryObj<DiracWilsonLike> Kernel;
  RaiiFactoryObj<Fopr_DdagD> HermitianOp;
  RaiiFactoryObj<Solver> Solv;
 
  const XML::node Action_node;

public:
  TwoFlavorEvenOddActionFactory(XML::node node)
    :Action_node(node){
    XML::descend(node,"Kernel", MANDATORY);
    DiracObj.save(DiracOperators::createDiracWilsonLikeOperatorFactory(node));
    XML::next_sibling(node,"Solver", MANDATORY);
    SolverObj.save(SolverOperators::createSolverOperatorFactory(node));
  }

  ~TwoFlavorEvenOddActionFactory(){}
private:
  
  Action_Nf2_EvenOdd* getFermionAction(const Format::Format_G& Form,
				       Field* const GaugeField){
    Kernel.save(DiracObj.get()->getDiracOperator(GaugeField));
    HermitianOp.save(new Fopr_DdagD(Kernel.get()));
    Solv.save(SolverObj.get()->getSolver(HermitianOp.get()));
    return new Action_Nf2_EvenOdd(GaugeField, Kernel.get(), Solv.get());
  }
};
*/

//Add new factories here
//....


////////////////////////////////////////////////////
namespace FermionAction {
  FermionActionFactory* createFermionActionFactory(XML::node);

}

#endif
