//--------------------------------------------------------------------
/*! @file hmcGeneral.cpp
 *
 * @brief Definitions of functions for HMC update
 *
 */
//--------------------------------------------------------------------
#include "hmcGeneral.hpp"
#include "mdExec.h"
#include "Communicator/comm_io.hpp"
#include "Communicator/fields_io.hpp"

#include <string>
#include <sstream>
class Field;


void HMCgeneral::evolve(Field& Uin)const{
  double Hdiff;
  // Thermalizations
  for(int iter=1; iter <= Params.ThermalizationSteps; ++iter){
    CCIO::cout << "-- # Thermalization step = "<< iter <<  "\n";

    Hdiff = evolve_step(Uin);
    CCIO::cout<< "dH = "<<Hdiff << "\n";
    Uin = md_->get_U();  //accept every time
  }

  // Actual updates
  for(int iter=1; iter <= Params.Nsweeps; ++iter){
    CCIO::cout << "-- # Sweep = "<< iter <<  "\n";
    CCIO::cout << "---------------------------\n";

    Hdiff = evolve_step(Uin);


    if(metropolis_test(Hdiff)) Uin = md_->get_U();

    if (Params.SaveInterval!=0 && iter%Params.SaveInterval == 0) {
      std::stringstream file;
      file << Params.Filename_prefix << iter;
      if(CCIO::SaveOnDisk<GaugeFieldFormat> (Uin, file.str().c_str())){
	CCIO::cout << "Some error occurred in saving file\n";
      }
    }


  }
}

double HMCgeneral::evolve_step(Field& Uin)const{
    
  std::vector<int> clock;
  md_->init(clock,Uin,*rand_);     // set U and initialize P and phi's 
  
  int init_level = 0;
  double H0 = md_->calc_H();     // current state            
  CCIO::cout<<"total H_before = "<< H0 << "\n";
  
  md_->integrator(init_level,clock);
  
  double H1 = md_->calc_H();     // updated state            
  CCIO::cout<<"Total H_after = "<< H1 << "\n";
  
  return (H1-H0);
}


bool HMCgeneral::metropolis_test(const double Hdiff)const{

  double prob = exp(-Hdiff);
  double rn = rand_->get();
  CCIO::cout<< "--------------------------------------------\n";
  CCIO::cout<< "dH = "<<Hdiff << "  Random = "<< rn 
	    << "\nAcc. Probability = " << ((prob<1.0)? prob: 1.0)<< "   ";
  if(prob >1.0){       // accepted
    CCIO::cout<<"-- ACCEPTED\n";
    return true;
  }else if(rn <= prob){// accepted
    CCIO::cout <<"-- ACCEPTED\n";
    return true;
  }else{               // rejected
    CCIO::cout <<"-- REJECTED\n";
    return false;
  }
}

