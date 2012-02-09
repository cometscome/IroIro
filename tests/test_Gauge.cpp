/*!
  @file test_Gauge.cpp
   
  @brief File for testing the gauge Field

 */

#include "test_Gauge.hpp"
#include "Measurements/GaugeM/staples.h"
#include "Communicator/comm_io.hpp"

using namespace std;
using namespace Format;

int Test_Gauge::run() {
  CCIO::header("Test Gauge");

  int res1 = plaquette();
  int res2 = shift();
  return (res1 && res2);

}

int Test_Gauge::plaquette(){
  Staples wl(d_conf.Format);
  double plaq = wl.plaquette(d_conf.U);

  CCIO::cout<<" Plaq = "<<plaq<<endl;
  return 0;
}

int Test_Gauge::shift(){
  CCIO::cout<<" Testing shifter:\n";
  ShiftField_up<Format_G> upt(d_conf.U,&(d_conf.Format),3); //+t
  ShiftField_dn<Format_G> umx(d_conf.U,&(d_conf.Format),0); //-x

  Staples wl(d_conf.Format);
  int nid = Communicator::instance()->nodeid();

  CCIO::cout <<" plaq (original) = "<<wl.plaquette(d_conf.U)<<" nodeid= "<<nid << endl;
  CCIO::cout <<" plaq (+t sfted) = "<<wl.plaquette(upt)     <<" nodeid= "<<nid << endl;
  CCIO::cout <<" plaq (-x sfted) = "<<wl.plaquette(umx)     <<" nodeid= "<<nid << endl;

  return 0;
}

