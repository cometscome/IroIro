//-------------------------------------------------------------------------
// randNum_MT19937.h
//-------------------------------------------------------------------------
#ifndef RANDNUM_MT19937_INCLUDED
#define RANDNUM_MT19937_INCLUDED

#ifndef RANDNUM_INCLUDED
#include "randNum.h"
#endif

#include<iostream>

#include <string>

class RandNum_MT19937 :public RandNum {
private:
  enum{N=624, M=397};

  mutable int left_;
  mutable unsigned long state_[N];
  mutable unsigned long* next_;

  unsigned long rand_int32()const;
  unsigned long twist(unsigned long u, unsigned long v)const;

  void init(unsigned long s);
  void init(unsigned long s, unsigned long* key, int key_length);
  void next_state()const;
  long rand_int31()const;

  double rand_double1()const;
  double rand_double2()const;
  double rand_double3()const;

  double rand_res53()const;
  double do_rand()const{ return rand_res53();}

public:
  RandNum_MT19937(unsigned long s = 5489UL):left_(1){init(s);}
  RandNum_MT19937(unsigned long* key,int key_length)
    :left_(1){init(19650218UL,key,key_length);}
  RandNum_MT19937(std::string& file,
		  unsigned long s = 5489UL):left_(1){
    init(s);
    loadSeed(file);
  }

  ~RandNum_MT19937(){
    std::string file("seed_file");
    saveSeed(file);}//saving seed when destroyed?

  void saveSeed(std::string& file);
  void loadSeed(std::string& file);

};

#endif
