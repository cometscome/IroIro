/*!
  @file action_gauge_rect.cpp
  @brief Definition of the ActionGaugeRect class
*/
#include "action_gauge_rect.hpp"
#include "Tools/sunMatUtils.hpp"
#include "Tools/fieldUtils.hpp"
#include "Measurements/GaugeM/staples.hpp"
#include "lib/Main/Geometry/mapping.hpp"
#include "include/messages_macros.hpp"

using namespace SUNmatUtils;
using namespace FieldUtils;

typedef struct GaugeConfigType{
  double _Complex v[9];
}GaugePtr;


double ActionGaugeRect::calc_H(){
  using namespace Mapping;

  int NP = CommonPrms::instance()->NP();
  double plaqF = 0.0;
  double rectF = 0.0;

  const Staples stpl_;
  GaugeField1D Cup1, Cup2;
  GaugeField1D U_nu, U_mu, res;
  GaugeField1D UpNu, UpMu;

#ifdef IBM_BGQ_WILSON
  double* UpMu_ptr = UpMu.data.getaddr(0);
  double* UpNu_ptr = UpNu.data.getaddr(0);
  double* res_ptr = res.data.getaddr(0);
  double* U_nu_ptr = U_nu.data.getaddr(0);
  double* U_mu_ptr = U_mu.data.getaddr(0);
  double* Cup1_ptr = Cup1.data.getaddr(0);
  double* Cup2_ptr = Cup2.data.getaddr(0);
#endif
  
  // From Matsufuru-san code
  for(int mu = 0; mu < NDIM_; ++mu){
    U_mu = DirSlice(*u_,mu); 
    
    for(int nu = mu+1; nu < NDIM_; ++nu){  
      Cup1 = stpl_.upper(*u_,mu,nu);
      Cup2 = stpl_.upper(*u_,nu,mu);
      
      // plaquette term
      for(int site=0; site<Nvol_; ++site){
	plaqF += ReTr(mat(*u_,site,mu)*mat_dag(Cup1,site) );
      }
      // rectangular terms
      //       mu,U_mu (UpNu) 
      //         +-->--+-->--+
      // nu,U_nu |           |Cup2_dag(site+mu) (UpMu)
      //     site+     +--<--+                               
      
      U_nu = DirSlice(*u_,nu);
      
      UpNu = shiftField(U_mu,nu,Forward());      //U_mu(x+nu)
      UpMu = shiftField(Cup2,mu,Forward());      //Cup2(x+mu)     
      
      res = 0.0;
#ifdef IBM_BGQ_WILSON
      BGWilsonSU3_MatMult_NND(res_ptr, U_nu_ptr, UpNu_ptr, UpMu_ptr, Nvol_);
      for(int site = 0; site<Nvol_; ++site){
	rectF += ReTr(mat(*u_,site,mu)*mat_dag(res,site));
      }
#else
      for(int site = 0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] = 
	  (mat(U_nu,site)*mat(UpNu,site)*mat_dag(UpMu,site)).getva();
	rectF += ReTr(mat(*u_,site,mu)*mat_dag(res,site));
      }
#endif
      
      //        Cup1(site+nu)
      //          +-->--+
      //          |     |
      //          +     +
      // nu,U_nu  |     | U_nu+mu (UpMu)
      //    site  +     +
      UpNu = shiftField(Cup1,nu,Forward());   //Cup1(x+nu)
      UpMu = shiftField(U_nu,mu,Forward());   //U_nu(x+mu)
      
      res = 0.0;
      for(int site = 0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] =
	  (mat(U_nu,site)*mat(UpNu,site)*mat_dag(UpMu,site)).getva();
	rectF += ReTr(mat(*u_,site,mu)*mat_dag(res,site));
      } 
    }
  }
  plaqF = Communicator::instance()->reduce_sum(plaqF);
  rectF = Communicator::instance()->reduce_sum(rectF);
  
  double plaq = plaqF/NC_;
  double rect = rectF/NC_;
  double Hgauge = Params.c_plaq*(Nvol_*NP*NDIM_*(NDIM_-1.0)/2.0 -plaq)
                + Params.c_rect*(Nvol_*NP*NDIM_*(NDIM_-1.0)     -rect);
  Hgauge *= Params.beta;
    
  _Message(ACTION_VERB_LEVEL, "    [ActionGaugeRect] H = "<<Hgauge<<"\n");
  _Message(1,"    -- Plaquette = "
	   << plaq/(Nvol_*NP*NDIM_*(NDIM_-1.0)/2.0) << "\n");
  return Hgauge;
}

GaugeField ActionGaugeRect::md_force(){
 using namespace Mapping;
  
  const Staples stpl_;
  SUNmat force_mat;
  GaugeField force;
  GaugeField1D force_pl;
  GaugeField1D force_rect;

  GaugeField1D Cup1;
  GaugeField1D Cup2;
  GaugeField1D Cdn1;
  GaugeField1D Cdn2;

  //check speed
  GaugeField1D U_mu, U_nu, res;
  GaugeField1D UpMu, UpNu;

  // GaugeField1D temp;
#ifdef IBM_BGQ_WILSON
  GaugePtr* force_rect_ptr = (GaugePtr*)force_rect.data.getaddr(0);
  double* UpMu_ptr = UpMu.data.getaddr(0);
  double* UpNu_ptr = UpNu.data.getaddr(0);
  double* res_ptr = res.data.getaddr(0);
  double* U_ptr = u_->data.getaddr(0);
  double* U_nu_ptr = U_nu.data.getaddr(0);
  double* U_mu_ptr = U_mu.data.getaddr(0);
  double* Cdn1_ptr = Cdn1.data.getaddr(0);
  double* Cdn2_ptr = Cdn2.data.getaddr(0);
  double* Cup2_ptr = Cup2.data.getaddr(0);
  double* Cup1_ptr = Cup1.data.getaddr(0);
#endif

  for(int mu=0; mu<NDIM_; ++mu){
    force_pl   = 0.0;
    force_rect = 0.0;

    U_mu = DirSlice(*u_, mu);   //U_mu links

    for(int nu=0; nu<NDIM_; ++nu){
      if (nu == mu) continue;

#ifdef IBM_BGQ_WILSON      
      Cup1 = stpl_.upper(*u_,mu,nu);
      Cup2 = stpl_.upper(*u_,nu,mu);
      Cdn1 = stpl_.lower(*u_,mu,nu);
      Cdn2 = stpl_.lower(*u_,nu,mu);

      // plaquette term
      force_pl += Cup1;
      force_pl += Cdn1;

      // lot of reorderings have been made

      DirSliceBGQ(U_nu, *u_, nu);   //U_nu links    

      shiftField(UpMu, Cup2_ptr, mu, Forward()); // Cup2(x+mu)
      shiftField(UpNu, U_mu_ptr, nu, Forward()); // U_mu(x+nu)

      BGWilsonSU3_MatMult_NND(res_ptr, U_nu_ptr, UpNu_ptr, UpMu_ptr, Nvol_);

      force_rect += res; 

      BGWilsonSU3_MatMult_DNN(Cup2_ptr, U_nu_ptr, U_mu_ptr, UpMu_ptr, Nvol_);
      
      shiftField(UpMu, U_nu_ptr, mu, Forward());    // U_nu(x+mu)
      shiftField(UpNu, Cup1_ptr, nu, Forward());    // Cup1(x+nu)
      
      BGWilsonSU3_MatMult_NND(res_ptr, U_nu_ptr, UpNu_ptr, UpMu_ptr, Nvol_);
      force_rect += res;     

      BGWilsonSU3_MatMult_DNN(res_ptr, U_nu_ptr, Cdn1_ptr, UpMu_ptr, Nvol_);
      BGWilsonSU3_MatMult_DNN(UpNu_ptr, Cdn2_ptr, U_mu_ptr, UpMu_ptr, Nvol_);
      res += UpNu;
      res += Cup2;
      force_rect += shiftField(res,nu,Backward());//+=res(x-nu)  

      shiftField(UpNu, U_mu_ptr, nu, Forward()); // U_mu(x+nu)
      BGWilsonSU3_MatMult_NND(res_ptr, Cdn2_ptr, UpNu_ptr, UpMu_ptr, Nvol_);

      force_rect += res;
#else
      Cup1 = stpl_.upper(*u_,mu,nu);
      Cup2 = stpl_.upper(*u_,nu,mu);
      Cdn1 = stpl_.lower(*u_,mu,nu);
      Cdn2 = stpl_.lower(*u_,nu,mu);

      // plaquette term
      force_pl += Cup1;
      force_pl += Cdn1;

      // rectangular terms
      // ^nu
      // |  
      // +-->mu
      //
      // (x) is the site position
      U_nu = DirSlice(*u_, nu);   //U_nu links    
      //          U_mu
      //         +-->--+-->--+
      //   U_nu  |           |   term  (Cup2)
      //        (x)    +--<--+      


      UpMu = shiftField(Cup2, mu, Forward()); // Cup2(x+mu)
      UpNu = shiftField(U_mu, nu, Forward()); // U_mu(x+nu)

      res = 0.0;
      for(int site=0; site<Nvol_; ++site){
        res.data[res.format.cslice(0,site)] = 
          (mat(U_nu,site)*mat(UpNu,site)*mat_dag(UpMu,site)).getva();
      }

      force_rect += res;  

  
      //         +-->--+
      //         |     |
      //         +     +   term
      //   U_nu  |     |  U_nu(x+mu) (UpMu)
      //        (x)    v
      UpMu = shiftField(U_nu, mu, Forward());    // U_nu(x+mu)
      UpNu = shiftField(Cup1, nu, Forward());    // Cup1(x+nu)
      res = 0.0;

      for(int site=0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] =
	  (mat(U_nu,site)*mat(UpNu,site)*mat_dag(UpMu,site)).getva();
      }       
     force_rect += res;     
      //           U_mu(x+nu)
      //      +-->--+-->--+
      //      |           |   term
      //      +--<-(x)    v
      UpNu = shiftField(U_mu, nu, Forward()); // U_mu(x+nu)
      res = 0.0; 

      for(int site=0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] = 
	  (mat(Cdn2,site)*mat(UpNu,site)*mat_dag(UpMu,site)).getva();
      }       
      force_rect += res;
      //     (x)    +--<--+
      //      |           |   term
      //      +-->--+-->--+
      UpMu = shiftField(Cup2, mu, Forward());

      res = 0.0;

      for(int site=0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] = 
	  (mat_dag(U_nu,site)*mat(U_mu,site)*mat(UpMu,site)).getva();
      } 
      force_rect += shiftField(res,nu,Backward()); //+=res(x-nu)
      //     (x)    ^
      //      |     |
      //      +     +   term
      //      |     |
      //      +-->--+
      UpMu = shiftField(U_nu, mu, Forward()); //U_nu(x+mu)

      res = 0.0;

      for(int site=0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] = 
	  (mat_dag(U_nu,site)*mat(Cdn1,site)*mat(UpMu,site)).getva();
      } 
      force_rect += shiftField(res,nu,Backward());//+=res(x-nu)    
      //      +--<-(x)    ^
      //      |           |   term
      //      +--<--+--<--+
      res = 0.0;

      for(int site = 0; site<Nvol_; ++site){
	res.data[res.format.cslice(0,site)] = 
	  (mat_dag(Cdn2,site)*mat(U_mu,site)*mat(UpMu,site)).getva();
      }       
      force_rect += shiftField(res,nu,Backward());//+=res(x-nu)  
#endif
  
    }
    force_pl   *= Params.c_plaq;
    force_rect *= Params.c_rect;
    force_rect += force_pl; //force_rect = total force (staples term)

#ifdef IBM_BGQ_WILSON 
    BGWilsonSU3_MatMult_ND(Cdn1_ptr , u_->data.getaddr(0)+18*Nvol_*mu, 
			   force_rect.data.getaddr(0), Nvol_);
    SetSlice(force, TracelessAntihermite(Cdn1), mu);
#else
    for(int site = 0; site<Nvol_; ++site){
      force_mat = (mat(*u_,site,mu)*mat_dag(force_rect,site));
      SetMat(force, anti_hermite_traceless(force_mat), site, mu);
    } 
#endif

  }
  force *= 0.5*Params.beta/NC_;
  _MonitorMsg(ACTION_VERB_LEVEL,Action,force,"ActionGaugeRect");

  return force;
}
