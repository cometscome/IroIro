//------------------------------------------------------------------------
/*!
 * @file test_HMC_noFactories.cpp
 *
 * @brief run() function for HMCgeneral class test without factories
 *
 * @author Jun-Ichi Noaki
 * @author <a href="http://suchix.kek.jp/guido_cossu/">Guido Cossu</a>
 */
//------------------------------------------------------------------------

#include <stdio.h>
#include <time.h>

#include "HMC/hmcGeneral.hpp"
#include "test_HMC.hpp"
#include "Tools/randNum_Factory.hpp"
#include "Action/action_gauge_wilson.hpp"
#include "Action/action_gauge_rect.hpp"
#include "Action/action_Nf2.hpp"
#include "HMC/mdExec_leapfrog.hpp"
#include "Dirac_ops/dirac_wilson.hpp"
#include "Solver/solver_CG.hpp"

int Test_HMC::run(){
  CCIO::cout << "Starting HMCrun\n";
  
  //Using factories just for RNG
  //RNG_Env::RNG = RNG_Env::createRNGfactory(HMC_node);
  RNG_Env::initialize(HMC_node);

  std::vector<int> multip(2);
  multip[0]= 1;
  multip[1]= 2;
  
  GaugeField* CommonField = new GaugeField;

  Action* Gauge = new ActionGaugeWilson(5.0, CommonField);
  //Action* Gauge = new ActionGaugeRect(2.25, 3.648, -0.331, CommonField);

  DiracWilsonLike* OpNf2    = new Dirac_Wilson(0.1,&(CommonField->data));
  
  ActionLevel al_1, al_2;
  al_1.push_back(Gauge);
   
  Solver* SolvNf2 = new Solver_CG(1e-14,
				  1000,
				  new Fopr_DdagD(OpNf2));
  
  Action* Nf2Action = new Action_Nf2(CommonField,
				     OpNf2,
				     SolvNf2);
  
  al_2.push_back(Nf2Action);
  
  ActionSet ASet;
  ASet.push_back(al_2);
  ASet.push_back(al_1);
  
  MDexec* Integrator = new MDexec_leapfrog(8,
					   10,
					   0.02,
					   ASet,
					   multip,
					   CommonField);
				      
  HMCgeneral hmc_general(HMC_node, *Integrator);  
  ////////////// HMC calculation /////////////////
  try{
    CCIO::cout<< "HMC starts\n";
    hmc_general.evolve(Gfield_);
  }catch(const char* error){
    CCIO::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
  ////////////////////////////////////////////////  

  return 0;
}
