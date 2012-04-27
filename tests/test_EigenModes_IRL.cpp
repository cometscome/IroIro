//------------------------------------------------------------------------
// test_EigenModes_IRL.cpp
//------------------------------------------------------------------------
#include "test_EigenModes_IRL.hpp"
#include "EigenModes/eigenModes_IRL.hpp"
#include "Fields/field_expressions.hpp"
#include "EigenModes/sortEigen.h"
#include "include/fopr.h"
#include "include/fopr_chebyshev_DdagD.h"
#include "Dirac_ops/dirac_wilson.hpp"
#include <stdio.h>

using namespace std;
using namespace FieldExpression;

int Test_EigenModes_IRL::run() {
  
  CCIO::header("Test EigenModes");
  //CCIO::cout<<"*********** lowlying **************"<<endl;
  //lowlying();
  //CCIO::cout<<"highest"<<endl;
  //highest();
  CCIO::cout<<"********** chebyshev ************"<<endl;
  chebyshev();

  return 0;
 }

int Test_EigenModes_IRL::lowlying(){

  Format::Format_F ff(CommonPrms::instance()->Nvol());

  //double mq  = 0.1666666666666;
  double mq  = -1.6;
  Fopr_H Hw(new Dirac_Wilson(mq, &(u_.data)));
  SortEigen_low sort;
  int    Nk = 20;
  int    Np = 50;
  double enorm = 1.e-22;
  double vthrs = 0.15;
  int    Niter = 500;

  EigenModes_IRL eigen(&Hw,&sort,Nk,Np,enorm,vthrs,Niter);
  Field b(ff.size());

  int Nmm = 100;
  int Nsbt = -1;
  int Nconv = -100;

  vector<double> lmd(Nmm);
  vector<Field>  evec(Nmm);
  for(int k=0; k<Nmm; ++k) evec[k].resize(ff.size());

  eigen.calc(lmd,evec,b,Nsbt,Nconv);

  Field v(ff.size());
  for(int i=0; i<Nsbt+1; ++i){
    v = Hw.mult(evec[i]);
    v -= lmd[i]*evec[i];
    CCIO::cout << "["<<i<<"] "<< lmd[i] << "  "<<v*v<<std::endl;
  }
  return 0;
}

int Test_EigenModes_IRL::highest(){
  
  Format::Format_F ff(CommonPrms::instance()->Nvol());
  double mq  = 0.166666666666;
  
  Fopr_H Hw(new Dirac_Wilson(mq,&(u_.data)));
  SortEigen_high sort;
  int    Nk = 20;
  int    Np = 20;
  double enorm = 1.e-22;
  double vthrs = 10.0;
  int    Niter = 500;

  EigenModes_IRL eigen(&Hw,&sort,Nk,Np,enorm,vthrs,Niter);
  Field b(ff.size());

  int Nmm = 100;
  int Nsbt = -1;
  int Nconv = -100;

  vector<double> lmd(Nmm);
  vector<Field>  evec(Nmm);
  for(int k=0; k<Nmm; ++k){evec[k].resize(ff.size());}

  eigen.calc(lmd,evec,b,Nsbt,Nconv);

  Field v(ff.size());
  for(int i=0; i<Nsbt+1; ++i){
    v = Hw.mult(evec[i]);
    v -= lmd[i]*evec[i];
    CCIO::cout << "["<<i<<"] "<< lmd[i] << "  "<<v*v<<std::endl;
  }
  
  return 0;
}

int Test_EigenModes_IRL::chebyshev()
{
  
  Format::Format_F ff(CommonPrms::instance()->Nvol());

  int Npoly = 40;
  double vthrs = 0.16;
  double vmax = 2.50;
  double mq  = -1.6;

  DiracWilsonLike* Kernel = new Dirac_Wilson(mq, &(u_.data));

  Fopr_Chebyshev_DdagD Tn_DdagD(Npoly,
				vthrs,
				vmax,
				Kernel);
  int    Nk = 20;
  int    Np = 50;
  double enorm_eigen = 1.e-22;
  double vthrs_eigen = 0.15;
  int    Niter_eigen = 500;

  double Tn_vthrs = Tn_DdagD.mult(vthrs_eigen);
  CCIO::cout << "Tn_vthrs = "<<Tn_vthrs<<std::endl;

  SortEigen_low sort;

  EigenModes_IRL eigen(&Tn_DdagD,&sort,Nk,Np,enorm_eigen,
		       Tn_vthrs,Niter_eigen);
  int Nmm = 100;
  int Nsbt = -1;
  int Nconv = -100;

  Field b(ff.size());
  vector<double> lmd(Nmm);
  vector<Field>  evec(Nmm);
  for(int k=0; k<Nmm; ++k) evec[k].resize(ff.size());

  eigen.calc(lmd,evec,b,Nsbt,Nconv);

  Fopr_H Hw(Kernel);

  for(int i=0; i<Nsbt+1; ++i){
    Field v = Hw.mult(evec[i]);
    double vnum = evec[i]*v;
    double vden = evec[i]*evec[i];
    lmd[i] = vnum/vden;
    v -= lmd[i]*evec[i];
    printf(" %4d %20.14f  %10.4e  %10.4e\n",i,lmd[i],v*v,vden-1.0);
  }
  

  return 0;

}

