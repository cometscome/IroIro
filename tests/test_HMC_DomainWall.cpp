//------------------------------------------------------------------------
/*!
 * @file test_HMC_DomainWall.cpp
 *
 * @brief run() function for HMCgeneral class test
 *
 * @author Jun-Ichi Noaki
 * @author <a href="http://suchix.kek.jp/guido_cossu/">Guido Cossu</a>
 */
//------------------------------------------------------------------------

#include <stdio.h>
#include <time.h>
#include <vector>

#include "HMC/hmcGeneral.h"
#include "test_HMC_DomainWall.hpp"
#include "Action/action_gauge.hpp"
#include "Action/action_Nf2_DomainWall.h"
#include "Communicator/comm_io.hpp"

using namespace std;

int Test_HMC_DomainWall::run(XML::node node){
  CCIO::cout << "Starting HMCrun" << std::endl;
 
  RNG_Env::RNG = RNG_Env::createRNGfactory(node);
  
  std::vector<int> multip(3);
  multip[0]= 1;
  multip[1]= 2;
  multip[2]= 2;
  
  Field CommonField(Gfield_.Format.size());

  int N5d = 6;
  double mzero = -1.8;
  double b = 2.0;
  double c = 0.0;
  double mq1 = 0.05;
  double mq2 = 0.10;
  vector<double> omega(N5d,1.0);
  Dirac_Wilson Kernel(mzero,&CommonField);
  
  Dirac_optimalDomainWall Ddwf1(b,c,mq1,omega,&Kernel);
  Dirac_optimalDomainWall Ddwf2(b,c,mq2,omega,&Kernel);

  // gauge term
  ActionLevel al_1, al_2, al_3;
  Action* Gauge 
    = new ActionGaugeWilson(6.0,Gfield_.Format,&CommonField);
  al_1.push_back(Gauge);

  // pf1 term
  Fopr_DdagD DdagD2(&Ddwf2);
  Solver_CG SolvNf2(10e-12,1000,&DdagD2);
  Action* Nf2Action 
    = new Action_Nf2_DomainWall(&CommonField,&Ddwf2,&SolvNf2);
  al_2.push_back(Nf2Action);

  // pf2 term
  Fopr_DdagD DdagD1(&Ddwf1);
  Solver_CG SolvR1(10e-12,1000,&DdagD1);
  Solver_CG SolvR2(10e-12,1000,&DdagD2);
  Action* RatioAction 
    = new Action_Nf2_ratio(&CommonField,&Ddwf1,&Ddwf2,&SolvR1,&SolvR2);
  al_3.push_back(RatioAction);

  ActionSet ASet;
  ASet.push_back(al_3);
  ASet.push_back(al_2);
  ASet.push_back(al_1);
  
  MDexec_leapfrog Integrator(8,3,0.00004,ASet,multip,
			     Gfield_.Format,&CommonField);

  HMCgeneral hmc_general(node,Integrator);  

  ////////////// HMC calculation /////////////////
  clock_t start_t = clock();
  try{
    CCIO::cout<< "HMC starts"<<std::endl;
    hmc_general.evolve(Gfield_.U);
  }catch(const char* error){
    CCIO::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
  clock_t end_t = clock();
  CCIO::cout << (double)(end_t -start_t)/CLOCKS_PER_SEC 
	     << std::endl;
  return 0;
}
