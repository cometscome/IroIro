/*! @file measGeneral.cpp
 *  @brief implementing member functions of the MeasGeneral class
 */
#include "measGeneral.hpp"
#include "GaugeM/staples.hpp"
#include "GaugeM/gfixFactory.hpp"
#include "Smearing/smearingFactories.hpp"
#include "Communicator/fields_io.hpp"
#include <iostream>
using namespace std;

void MeasGeneral::list_config(){
  XML::node config_node= node_;
  XML::descend(config_node,"Configuration");
  for(XML::iterator it=config_node.begin(); it!=config_node.end();++it)
    config_list_.push_back(it->child_value());
}

void MeasGeneral::pre_process(GaugeField& U,const RandNum& rng,int id){

  Staples Staple;
  CCIO::cout<< "Plaquette (thin): "<< Staple.plaquette(U) <<"\n";

  GaugeField Ubuf = U;

  //// smearing ////
  XML::node smr_node = node_; 
  XML::descend(smr_node,"Smearing"); 

  int Nsmear;                                    
  XML::read(smr_node,"Nsmear",Nsmear,MANDATORY);  

  SmearingOperatorFactory* SmrFactory = 
    SmearingOperators::createSmearingOperatorFactory(smr_node);
  Smear* SmearingObj = SmrFactory->getSmearingOperator();

  for(int i=0; i<Nsmear; ++i){ // Do the actual smearing 
    GaugeField previous_u = Ubuf;
    SmearingObj->smear(Ubuf,previous_u);
  }
  CCIO::cout<<"Plaquette (smeared): "<<Staple.plaquette(Ubuf)<<endl;
  
  //// gauge fixing ////
  XML::node gfix_node = node_;
  XML::descend(gfix_node,"GaugeFixing");
  GFixFactory* gffct = GaugeFix::createGaugeFixingFactory(gfix_node);
  GaugeFixing* gfix = gffct->getGaugeFixing(rng);
  
  U = gfix->do_fix(Ubuf);

  CCIO::cout<<"Plaquette (gauge fixed): "<<Staple.plaquette(U)<<endl;

  if(gauge_output_){
    std::stringstream gout;
    gout << gauge_prefix_<< id;
    if(CCIO::SaveOnDisk<Format::Format_G> (U.data,gout.str().c_str()))
      CCIO::cout << "Some error occurred in saving file\n";
  }
}

void MeasGeneral::post_process(GaugeField&,const RandNum& rng,int id){
  if(seed_output_){
    std::stringstream seed;
    seed << seed_prefix_<< id;
    rng.saveSeed(seed.str());
  }
}
