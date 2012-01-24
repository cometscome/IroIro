/*!
 * @file test_smear.cpp
 *
 * @brief Tests for the propagators 
 *
 */
#include "test_smear.hpp"
#include "Measurements/FermionicM/fermion_meas_factory.hpp"


#include "Dirac_ops/dirac_wilson.h"
#include "Solver/solver_CG.h"
#include "Solver/solver_BiCGStab.h"
#include "Measurements/FermionicM/qprop.h"
#include "Measurements/FermionicM/source_types.hpp"
#include "Measurements/FermionicM/meson_correlator.hpp"
#include "Smearing/StoutSmear.hpp"
#include <stdio.h>

using namespace std;
using namespace Format;

int Test_Smear::run(){
  prop_t sq;// propagator
  // Use factories to construct the propagator
  QuarkPropagator* QP;
  XML::descend(Smear_node_, "QuarkPropagator");
  QuarkPropagatorFactory* QP_Factory = 
    QuarkPropagators::createQuarkPropagatorFactory(Smear_node_);
  /////////////////////////////////////////////

  // Just a check on configuration
  Staples Staple(conf_.Format);
  CCIO::cout << "Plaquette : " << Staple.plaquette(conf_.U) << std::endl;
  //////////////////////////////////////
  // source 
  vector<int> source_pos(4,0); 

  Source_local<Format::Format_F> src(source_pos,
				     CommonPrms::instance()->Nvol());

  // Without factories -----------------------------------------------------
  // Dirac Kernel definition
  // Create smearing objects
  Smear_APE BaseAPE(conf_.Format);
  Smear_Stout AnalyticSmear(BaseAPE, conf_.Format);
  smeared_u_ = conf_; // Copy thin links to the initial smearing
  int Nsmear = 2;

  gauge_pointer = &(smeared_u_.U);
  Dirac* Kernel = new Dirac_Wilson(1.0/6.0, gauge_pointer);
  
  // Solver definition
  int    Niter= 1000;
  double stop_cond = 1.0e-24;
  Solver_CG* SolverCG = new Solver_CG(stop_cond,
				      Niter,
				      new Fopr_DdagD(Kernel));


  Qprop QuarkPropagator(Kernel,SolverCG);
  MesonCorrelator meson(Pion);

  // Smearing and quark propagator

  for(int smear_step = 0; smear_step <= Nsmear; ++smear_step){
    
    CCIO::cout << "Smearing step #"<<smear_step<<"\n";
    
    if ( smear_step > 0) {
      previous_u_ = smeared_u_;
      AnalyticSmear.smear(smeared_u_.U, previous_u_.U);
    }
    
 
    CCIO::cout << "Plaquette : " << Staple.plaquette(smeared_u_.U) << std::endl;
   
    QuarkPropagator.calc(sq,src);
    
    
    
    //---------------------------------------------------------------------------
    // With factories -- comment out to use 
    // QP = QP_Factory->getQuarkProp(conf_);
    // QP->calc(sq,src);
    
    CCIO::cout<<"Quark propagator obtained\n";
    
    // meson correlators

  vector<double> mcorr = meson.calculate<Format::Format_F>(sq,sq);  
  vector<double>::const_iterator it=mcorr.begin();
  int t=0;
  std::cout << scientific; 
  while(it!=mcorr.end()) CCIO::cout << t++ << "  "<< *it++ << "\n";
  }

  return 0;
}
