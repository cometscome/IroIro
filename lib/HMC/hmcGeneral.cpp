//--------------------------------------------------------------------
/*! @file hmcGeneral.cpp
 *
 * @brief Definitions of functions for HMC update
 *
 */
//--------------------------------------------------------------------
#include "hmcGeneral.hpp"
#include "Communicator/comm_io.hpp"
#include "Communicator/fields_io.hpp"

#include <string>
#include <sstream>

void HMCgeneral::evolve(GaugeField& Uin)const{
  double Hdiff;
  std::string seedfile_base = "seed_file_";
  std::string seedfile;
  std::stringstream seedfile_string;
  // Thermalizations
  for(int iter=1; iter <= Params.ThermalizationSteps; ++iter){
    CCIO::cout << "-- # Thermalization step = "<< iter <<  "\n";

    double timer;
    TIMING_START;
    Hdiff = evolve_step(Uin);
    TIMING_END(timer);
    
    CCIO::cout<< "Time for trajectory (s) : "<< timer/1000.0 << "\n";
    CCIO::cout<< "dH = "<< Hdiff << "\n";
    Uin = md_->get_U();  //accept every time
  }

  // Actual updates
  for(int iter=Params.StartingConfig; 
      iter < Params.Nsweeps+Params.StartingConfig; ++iter){
    CCIO::cout << "-- # Sweep = "<< iter <<  "\n";
    CCIO::cout << "---------------------------\n";
    double timer;
    TIMING_START;
    Hdiff = evolve_step(Uin);
    TIMING_END(timer);
    CCIO::cout<< "Time for trajectory (s) : "<< timer/1000.0 << "\n";

    if(metropolis_test(Hdiff)) Uin = md_->get_U();

    if (Params.SaveInterval!=0 && iter%Params.SaveInterval == 0) {
      std::stringstream file;
      file << Params.Filename_prefix << iter;
      if(CCIO::SaveOnDisk<Format::Format_G> (Uin.data, file.str().c_str())){
	CCIO::cout << "Some error occurred in saving file\n";
      }
      seedfile_string.str("");//clears up the string
      seedfile_string << seedfile_base << iter;
      rand_->saveSeed(seedfile_string.str());
    }
  }
  seedfile_string.str("");//clears up the string
  seedfile_string << seedfile_base << "last";
  rand_->saveSeed(seedfile_string.str());
}

double HMCgeneral::evolve_step(GaugeField& Uin)const{
    
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

