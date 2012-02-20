//----------------------------------------------------------------------
/*! @file staples.hpp
  
  @brief Defines the staples measurement classes

*/
//----------------------------------------------------------------------
#include "staples.hpp"
#include "Tools/sunMatUtils.hpp"



using namespace SUNmat_utils;
using namespace FieldUtils;
typedef ShiftField_up<GaugeFieldFormat> FieldUP;
typedef ShiftField_dn<GaugeFieldFormat> FieldDN;
typedef std::valarray<double> field1d;

//------------------------------------------------------------
double Staples::plaquette(const GaugeFieldType& F)const {
  return (plaq_s(F) + plaq_t(F))*0.5;
}
//------------------------------------------------------------
double Staples::plaq_s(const GaugeFieldType& F)const {
  double plaq = 0.0;
  GaugeField1DType stpl;

  for(int i=0;i<NDIM_-1;++i){
    int j = (i+1)%(NDIM_-1);
    
    stpl = lower(F,i,j);

    for(int site=0; site<Nvol_; ++site)
      plaq += ReTr(matrix(F,site,i) * matrix_dag(stpl,site));  // P_ij
  }
  plaq = com_->reduce_sum(plaq);
  return plaq/(Lvol_*Nc_*3.0);
}
//------------------------------------------------------------
double Staples::plaq_t(const GaugeFieldType& F)const {
  double plaq = 0.0;
  GaugeField1DType stpl;

  for(int nu=0; nu < NDIM_-1; ++nu){
    stpl = lower(F,3,nu);
    for(int site=0; site<Nvol_; ++site)
      plaq += ReTr(matrix(F,site,3)* matrix_dag(stpl,site));  // P_zx
  }
  plaq = com_->reduce_sum(plaq);
  return plaq/(Lvol_*Nc_*3.0);
}
//------------------------------------------------------------
GaugeField1DType Staples::lower(const GaugeFieldType& G, int mu, int nu) const{
  //         +     +
  // nu,w_dag|     |w(site+mu,nu) 
  //     site+-->--+ 
  //           mu,v              
  GaugeField1DType v = DirSlice(G,mu);
  GaugeField1DType w = DirSlice(G,nu);

  GaugeField1DType c;
  GaugeField1DType WupMu = shift(w,mu,Forward);

  for(int site = 0; site < Nvol_; ++site) 
    c.data[c.format.cslice(0,site)] = 
      (matrix_dag(w,site)* matrix(v,site)* matrix(WupMu,site)).getva();

  return shift(c,nu,Backward);
}
//------------------------------------------------------------
GaugeField1DType Staples::upper(const GaugeFieldType& G, int mu, int nu) const{

  //       mu,v                               
  //      +-->--+                                                    
  // nu,w |     |t_dag(site+mu,nu)
  //  site+     +                                                             

  GaugeField1DType v = DirSlice(G,mu);
  GaugeField1DType w = DirSlice(G,nu);

  GaugeField1DType c;
  GaugeField1DType WupMu = shift(w,mu,Forward);
  GaugeField1DType VupNu = shift(v,nu,Forward);
  for(int site = 0; site < Nvol_; ++site){
    c.data[c.format.cslice(0,site)] = 
      (matrix(w,site) * matrix(VupNu,site) * matrix_dag(WupMu,site)).getva();
  }
 
  return c;
}
//------------------------------------------------------------
void Staples::staple(GaugeField1DType& W, const GaugeFieldType& G, int mu) const{
  W = 0.0;
  for(int nu = 0; nu < NDIM_; nu++){
    if(nu != mu){
      W += upper(G,mu,nu);
      W += lower(G,mu,nu);
    }
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

//------------------------------------------------------------
double Staples::plaquette(const Field& g)const{ 
  return (plaq_s(g) +plaq_t(g))/2;
}
//------------------------------------------------------------
double Staples::plaquette(const ShiftField& gs)const{
  Field g(gs.getva());
  return (plaq_s(g) +plaq_t(g))/2;
}
//------------------------------------------------------------
double Staples::plaq_s(const Field& g) const{
  double plaq = 0.0;
  //  Field stpl(sf_->size());
  GaugeField1D stpl;

  for(int i=0;i<Ndim_-1;++i){
    int j = (i+1)%(Ndim_-1);
    
    stpl.U = lower(g,i,j);

    for(int site=0; site<Nvol_; ++site)
      plaq += ReTr(u(g,gf_,site,i) * u_dag(stpl,site));  // P_ij
  }
  plaq = com_->reduce_sum(plaq);
  return plaq/(Lvol_*Nc_*3.0);
}
//------------------------------------------------------------
double Staples::plaq_s(const ShiftField& gs) const{
  Field g(gs.getva());
  return plaq_s(g);
}
//------------------------------------------------------------
double Staples::plaq_t(const Field& g)const{
  double plaq = 0.0;
  GaugeField1D stpl;

  for(int nu=0; nu < Ndim_-1; ++nu){
    stpl.U = lower(g,3,nu);
    for(int site=0; site<Nvol_; ++site)
      plaq += ReTr(u(g,gf_,site,3)*u_dag(stpl,site));  // P_zx
  }
  plaq = com_->reduce_sum(plaq);
  return plaq/(Lvol_*Nc_*3.0);
}
//------------------------------------------------------------
double Staples::plaq_t(const ShiftField& gs)const{
  Field g(gs.getva());
  return plaq_t(g);
}
//------------------------------------------------------------
void Staples::staple(Field& W, const Field& g, int mu) const{
  W = 0.0;
  for(int nu = 0; nu < Ndim_; nu++){
    if(nu != mu){
      W += upper(g,mu,nu);
      W += lower(g,mu,nu);
    }
  }
}
//------------------------------------------------------------
void Staples::staple(Field& W, const ShiftField& gs, int mu)const {
  Field g(gs.getva());
  staple(W,g,mu);
}
//------------------------------------------------------------
Field Staples::upper(const Field& g, int mu, int nu) const{

  //       mu,v                               
  //      +-->--+                                                    
  // nu,w |     |t_dag(site+mu,nu)
  //  site+     +                                                             

  field1d w = g[gf_.dir_slice(nu)];
  field1d v = g[gf_.dir_slice(mu)];
  FieldUP um(w,format1d_,mu);
  FieldUP un(v,format1d_,nu);

  field1d c(format1d_->size());
  for(int site=0; site<Nvol_; ++site){
    c[format1d_->cslice(0,site)] = (u(w,*format1d_,site)*u(un,site)*u_dag(um,site)).getva();
  }
  return Field(c);
}
//------------------------------------------------------------		 
Field Staples::upper(const ShiftField& gs, int mu,int nu) const{
  return upper(Field(gs.getva()),mu,nu);
}
//------------------------------------------------------------
Field Staples::lower(const Field& g, int mu, int nu) const{
  //         +     +
  // nu,w_dag|     |w(site+mu,nu) 
  //     site+-->--+ 
  //           mu,v              

  field1d v = g[gf_.dir_slice(mu)];
  field1d w = g[gf_.dir_slice(nu)];

  FieldUP um(w,format1d_,mu);

  field1d c(format1d_->size());
  for(int site=0; site<Nvol_; ++site)
    c[format1d_->cslice(0,site)] = (u_dag(w,*format1d_,site)*u(v,*format1d_,site)*u(um,site)).getva();

  FieldDN un(c,format1d_,nu);

  for(int site=0; site<Nvol_; ++site)
    v[format1d_->cslice(0,site)] = u(un,site).getva();
  return Field(v);
}
//------------------------------------------------------------
Field Staples::lower(const ShiftField& gs, int mu,int nu) const{
  return lower(Field(gs.getva()),mu,nu);
}

