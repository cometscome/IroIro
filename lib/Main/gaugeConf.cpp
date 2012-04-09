//-------------------------------------------------------------------------
// gaugeConf.cpp
//-------------------------------------------------------------------------
#include <vector>
#include <valarray>
#include "gaugeConf.hpp"
#include "include/field.h"
#include "Communicator/communicator.h"
#include "Communicator/fields_io.hpp"
#include "Communicator/comm_io.hpp"
#include "Tools/sunMat.hpp"
using namespace std;

int GaugeConf::getid(int x,int y,int z,int t){
  return Communicator::instance()->nodeid(x/Nx_,y/Ny_,z/Nz_,t/Nt_);
}

void GaugeConf_bin::init_conf(Field& U){
  CCIO::ReadFromDisk< Format::Format_G >(U,file.c_str());
}

void GaugeConf_JLQCDLegacy::init_conf(Field& U){
  CCIO::ReadFromDisk< Format::Format_G >(U,file.c_str(),
					 FORTRAN_CONTROL_WORDS, 
					 CCIO::JLQCDLegacyFormat);
}

void GaugeConf_txt::init_conf(Field& u){
  
  int Lx = CommonPrms::instance()->Lx();
  int Ly = CommonPrms::instance()->Ly();
  int Lz = CommonPrms::instance()->Lz();
  int Lt = CommonPrms::instance()->Lt();
  int Lvol = CommonPrms::instance()->Lvol();
  int Nvol = CommonPrms::instance()->Nvol();
  int NPEx = CommonPrms::instance()->NPEx();
  int NPEy = CommonPrms::instance()->NPEy();
  int NPEz = CommonPrms::instance()->NPEz();
  int NPEt = CommonPrms::instance()->NPEt();
  int NP  = CommonPrms::instance()->NP();
  
  Communicator* comm = Communicator::instance();
  int nodeid = comm->nodeid();
  
  vector<valarray<double>*> u_all;
  for(int id = 0; id < NP; ++id) 
    u_all.push_back(new valarray<double>(fg_.size()));
  valarray<double> cplx(2);  
  
  valarray<double> u_tmp(fg_.size());
  
  if(nodeid==0){
    fstream config; 
    try {
      config.open(file.c_str(), ios::in);
      if(! config.is_open())
	throw "Failed to open the configuration file.";
    }
    catch ( char const* str) {
      cout << "GaugeConf_txt Exception: " << str << endl;
      abort();
    }
    
    CCIO::cout<< "Reading gauge configuration from " << file.c_str() << endl;
    for(int t = 0; t < Lt; ++t){
      for(int z = 0; z < Lz; ++z){
        for(int y = 0; y < Ly; ++y){
          for(int x = 0; x < Lx; ++x){
	    
	    int id = getid(x,y,z,t);
            int site = SiteIndex::instance()->site(x%Nx_,y%Ny_,z%Nz_,t%Nt_);
            for(int dir = 0; dir < Ndim_; ++dir){
              for(int c1 = 0; c1 < Nc_; ++c1){
                for(int c2 = 0; c2 < Nc_; ++c2){
                  config >> cplx[0]; 
                  config >> cplx[1];
                  (*(u_all[id]))[fg_.cplx_slice(c1,c2,site,dir)] = cplx;
                }
              }
            }
          }	 
        }   
      }
    }
  }
  comm->sync();
  for(int id = 0; id < NP; ++id)
    comm->send_1to1(u_tmp, *(u_all[id]), u_tmp.size(), id, 0, id);
  comm->sync();
  
  u = Field(u_tmp);
  for(int id = 0; id < NP; ++id) delete u_all[id];
    
}
    
void GaugeConf_unit::init_conf(Field& u){
  for(int t = 0; t < Nt_; ++t){
    for(int z = 0; z < Nz_; ++z){
      for(int y = 0; y < Ny_; ++y){
        for(int x = 0; x < Nx_; ++x){

          int site = SiteIndex::instance()->site(x,y,z,t);

          for(int dir=0; dir<Ndim_; ++dir){
            for(int c=0; c<Nc_; ++c){
	      u.set(fg_.index_r(c,c,site,dir),1.0);
            }
          }
        }
      }
    }
  }
}

void GaugeConf_rand::init_conf(Field& Gfield){
  int Nvol= CommonPrms::instance()->Nvol();

  std::valarray<double> va(Gfield.size());
  MPrand::mp_get(va,rand_,SiteIndex::instance()->get_gsite(),fg_);

  for(int dir=0; dir<fg_.Nex(); ++dir){
    for(int site=0; site<fg_.Nvol(); ++site){
      SUNmat U(va[fg_.islice(site,dir)]);
      U.reunit();
      Gfield.set(fg_.islice(site,dir),U.getva());
    }
  }
}
