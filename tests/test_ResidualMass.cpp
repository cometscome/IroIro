/*!
 * @file test_ResidualMass.cpp
 *
 * @brief Definition of classes for calculating the Residual Mass
 *
 */
#include "test_ResidualMass.hpp"
#include "Communicator/comm_io.hpp"
#include "Communicator/fields_io.hpp"
#include "Dirac_ops/dirac_Operator_Factory.hpp"
#include "Tools/randNum_Factory.h"
#include "Measurements/FermionicM/qprop_DomainWall.hpp"
#include "Measurements/FermionicM/source_types.hpp"
#include "Measurements/FermionicM/sources_factory.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>
#include <ctime>

using namespace std;
using namespace Format;

const Field Test_ResMass::delta(const Dirac_optimalDomainWall_4D* DWF, Field& phi){
  //Delta function = 1/4 * (1- sign^2(Hw))
  Field sign = DWF->signKernel(phi);
  Field delta = DWF->signKernel(sign); //sign^2(Hw)
  delta -= phi;  //sign^2(Hw) -1
  
  delta *= -0.25; // 1/4*(1-sign^2(Hw))
  
  return delta;
}

int Test_ResMass::run(XML::node node) {
  RNG_Env::RNG = RNG_Env::createRNGfactory(node);
  
  XML::descend(node, "DiracOperator");
  // operator
  // here using a specific factory since we are testing the DWF-4d operator
  DiracDWF4dFactory* DWF_4d_Factory = new DiracDWF4dFactory(node);
  Dirac_optimalDomainWall_4D* DiracDWF_4d = DWF_4d_Factory->getDiracOperator(&(conf_.U));
  //Propagator
  QpropDWF QuarkPropagator(*DiracDWF_4d);

  XML::next_sibling(node, "Source");
  SourceFactory* Source_Factory = Sources::createSourceFactory<Format_F>(node);
  Source* SourceObj =  Source_Factory->getSource();

  prop_t sq;  //Defines a vector of fields
  CCIO::cout << "Calculating propagator\n";
  QuarkPropagator.calc(sq,*SourceObj);
 
  // Cycle among Dirac and color indexes and contract
  // D^-1 * Delta * D^-1
  double mres_numerator = 0;
  double im_check = 0;
  double mres_denominator = 0;
  Field Delta, Denom;

  for (int s = 0; s < 4; ++s) {
    for (int c = 0; c < 3; ++c) {
      Delta = delta(DiracDWF_4d,sq[c+3*s]); // (Delta * D^-1)*source
      //Contracting 
      mres_numerator += sq[c+3*s]*Delta;          // Re(sq[],Delta)    sq[]=D^-1*source
      im_check       += sq[c+3*s].im_prod(Delta); //should be always zero (just a check)
      CCIO::cout << "Numerator = ("<<mres_numerator<<","<<im_check<<")\n";
      
      //Denominator
      Denom = sq[c+3*s];
      Denom -= SourceObj->mksrc(s,c); // (D^-1 - 1)*src
      Denom /= (1.0 - DiracDWF_4d->getMass());
      mres_denominator += Denom*Denom;
      CCIO::cout << "Denominator = " << mres_denominator << endl;
      CCIO::cout << "Residual mass = " << mres_numerator/mres_denominator << endl;
    }
  }

  return 0;
}
