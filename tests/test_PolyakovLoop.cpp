/* @file test_PolyakovLoop.cpp
 * @brief implementation of the Test_PolyakovLoop.cpp class
 */
#include "Measurements/GaugeM/polyakovLoop.hpp"
#include "Measurements/GaugeM/staples.hpp"
#include "test_PolyakovLoop.hpp"
#include <complex>
#include <cstring>
#include <iomanip>

int Test_PolyakovLoop::run(){
  XML::node pnode = input_.node;
  XML::descend(pnode,"PolyakovLoop");
  const char* dir_name = pnode.attribute("dir").value();

  Staples Staple;
  CCIO::cout<< "Plaquette      : "<<  Staple.plaquette(*(input_.config.gconf))<< std::endl;
  CCIO::cout<<" Plaquette (xy) : "<<  Staple.plaq_mu_nu(*(input_.config.gconf), 0,1) << std::endl;
  CCIO::cout<<" Plaquette (xz) : "<<  Staple.plaq_mu_nu(*(input_.config.gconf), 0,2) << std::endl;
  CCIO::cout<<" Plaquette (xt) : "<<  Staple.plaq_mu_nu(*(input_.config.gconf), 0,3) << std::endl;
  CCIO::cout<<" Plaquette (yz) : "<<  Staple.plaq_mu_nu(*(input_.config.gconf), 1,2) << std::endl;
  CCIO::cout<<" Plaquette (yt) : "<<  Staple.plaq_mu_nu(*(input_.config.gconf), 1,3) << std::endl;
  CCIO::cout<<" Plaquette (zt) : "<<  Staple.plaq_mu_nu(*(input_.config.gconf), 2,3) << std::endl;

  site_dir dir;
  if(     !strcmp(dir_name,"X")) dir = XDIR;
  else if(!strcmp(dir_name,"Y")) dir = YDIR;
  else if(!strcmp(dir_name,"Z")) dir = ZDIR;
  else if(!strcmp(dir_name,"T")) dir = TDIR;
  else {
    CCIO::cout<<"No valid direction available with name "<< dir_name << "\n";
    abort();
  }
  PolyakovLoop plp(dir);

  std::complex<double> pf = plp.calc_SUN(*(input_.config.gconf));
  double            pa = plp.calc_SUNadj(*(input_.config.gconf));
  
  CCIO::cout<<"Polyakov Loop ("<<dir << ") [fundamental representation]: ";
  CCIO::cout<< std::setw(20)<<pf.real()<<" "<< std::setw(20)<<pf.imag()<<"\n";
  CCIO::cout<<"Polyakov Loop [adjoint representation]:     ";
  CCIO::cout<< std:: setw(20) << pa <<"\n";
  
  return 0;
}
