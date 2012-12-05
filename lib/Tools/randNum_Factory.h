/*! @file randNum_Factory.h
    @brief Defines the Factories for Random Number Generators
*/
#ifndef RANDNUM_FACT_
#define RANDNUM_FACT_

#include "randNum.h"
#include "randNum_MT19937.h"
#include "include/singleton.h"
#include "include/pugi_interface.h"
#include "Communicator/comm_io.hpp"
/*!
 *@class RandomNumberCreator
 *@brief Abstract Factory class for Random Number Generators
 */
class RandomNumberCreator {
public:
  /*! Virtual function returning the Random Number Generator */
  virtual RandNum* getRandomNumGenerator() = 0;
};

//Specific factories 
////////////////////////////////////////////////////////////////////
class RandNum_MT19937_Creator : public RandomNumberCreator {
  unsigned long *init_;
  int length_;
  std::string filename_;
  std::vector<unsigned long> inputs_;
  bool fromfile_;

  RandNum_MT19937* createRNG(){
    if(fromfile_) return new RandNum_MT19937(filename_);
    else          return new RandNum_MT19937(init_,length_); 
  }
public:
  explicit RandNum_MT19937_Creator(std::string filename)
    :filename_(filename),fromfile_(true){}

  RandNum_MT19937_Creator(unsigned long *in, int l)
    :init_(in),length_(l),fromfile_(false){}

  RandNum_MT19937_Creator(XML::node node){
    if(node.child("seedFile")!= NULL){
      fromfile_= true;
      XML::read(node,"seedFile",filename_,MANDATORY); 
    }else{
      fromfile_= false;
      inputs_.resize(4);
      XML::read_array(node,"init",inputs_,MANDATORY);
      init_= &inputs_[0];
      length_= inputs_.size();
    }
  }
  RandNum* getRandomNumGenerator(){ return createRNG();}
};

//////////////////////////////////////////////////////////////
namespace RNG_Env {
  static RandomNumberCreator* RNG;
  RandomNumberCreator* createRNGfactory(XML::node);  
}

#endif
