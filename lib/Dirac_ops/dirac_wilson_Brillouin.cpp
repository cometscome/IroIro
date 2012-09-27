//----------------------------------------------------------------------
// dirac_wilson_Brillouin.cpp
//----------------------------------------------------------------------
#include "dirac_wilson_Brillouin.hpp"
#include "Tools/sunMatUtils.hpp"
#include "Tools/sunVec.hpp"
#include "Fields/field_expressions.hpp"

using namespace SUNvecUtils;
using namespace std;

/////////////////////////////////////////////////////////////////////////

int Dirac_Wilson_Brillouin::sgm(int mu,int nu, int rho)const{
  for(int sigma=0; sigma<NDIM_; ++sigma){
    if(sigma !=mu && sigma !=nu && sigma !=rho) return sigma;
  }
}

const Field (Dirac_Wilson_Brillouin::*Dirac_Wilson_Brillouin::gm[])
(const Field&) const = {&Dirac_Wilson_Brillouin::gamma_x,
			&Dirac_Wilson_Brillouin::gamma_y,
			&Dirac_Wilson_Brillouin::gamma_z,
			&Dirac_Wilson_Brillouin::gamma_t,};

const Field Dirac_Wilson_Brillouin::mult_dag(const Field& f)const{ 
  return gamma5(mult(gamma5(f)));
}
const vector<int> Dirac_Wilson_Brillouin::get_gsite() const {
  return SiteIndex::instance()->get_gsite();
}

/*!@brief U_{x,\mu}\psi_{x+\hat{\mu}} */

const Field Dirac_Wilson_Brillouin::sft_p(const Field& f,int dir) const{
  Field fp(fsize_);
  int Ni = 2*ND_*NC_; /*!< @brief num of elements of a spinor */
  Format::Format_F ff = get_fermionFormat();
  
  /// boundary part ///
  int is = 0;
  int Xb = 0;
  int Nbdry = slsize(dir);
  double vbd[Ni*Nbdry]; /*!< @brief data on the lower slice */   
  for(int k=0; k<Nbdry; ++k){
    const double* v = const_cast<Field&>(f).getaddr(ff_.index(0,xsl(Xb,k,dir)));
    for(int s=0; s<ND_; ++s){
      for(int c=0; c<NC_; ++c){
	vbd[sr(s,c)+is] = v[sr(s,c)]; 
	vbd[si(s,c)+is] = v[si(s,c)];
      }
    }
    is += Ni;
  }
  double vbc[Ni*Nbdry]; /*!< @brief data on the upper slice */   
  comm_->transfer_fw(vbc,vbd,Ni*Nbdry,dir);

  is = 0;
  Xb = SiteIndex::instance()->Bdir(dir);
  Nbdry = slsize(dir);

  for(int k=0; k<Nbdry; ++k){  /*!< @brief calc on the upper boundary */   

    int xc = xsl(Xb,k,dir);
    const double* U = const_cast<Field*>(u_)->getaddr(gf_.index(0,xc,dir));
    double* res = fp.getaddr(ff_.index(0,xc));    
    
    for(int s=0; s<ND_; ++s){
      for(int c=0; c<NC_; ++c){
	double vr = 0.0, vi = 0.0;
      
	for(int c1=0; c1<NC_; ++c1){
	  vr += U[re(c,c1)]*vbc[sr(s,c1)+is] -U[im(c,c1)]*vbc[si(s,c1)+is];
	  vi += U[im(c,c1)]*vbc[sr(s,c1)+is] +U[re(c,c1)]*vbc[si(s,c1)+is];
	}
	res[sr(s,c)] += vr;
	res[si(s,c)] += vi;
      }
    }
    is += Ni;
  }
  /// bulk part ///
  for(int x=0; x<Xb; ++x){
    int Nslice = slsize(dir);
    for(int k=0; k<Nslice; ++k){   
      const double* v = const_cast<Field&>(f).getaddr(ff_.index(0,xsl(x+1,k,dir)));
      const double* U = const_cast<Field*>(u_)->getaddr(gf_.index(0,xsl(x,k,dir),dir));
      double* res = fp.getaddr(ff_.index(0,xsl(x,k,dir)));

      for(int s=0; s<ND_; ++s){
	for(int c=0; c<NC_; ++c){
	  double vr = 0.0, vi = 0.0;
	  
	  for(int c1=0; c1<NC_; ++c1){
	    vr += U[re(c,c1)]*v[sr(s,c1)] -U[im(c,c1)]*v[si(s,c1)];
	    vi += U[im(c,c1)]*v[sr(s,c1)] +U[re(c,c1)]*v[si(s,c1)];
	  }
	  res[sr(s,c)] += vr; 
	  res[si(s,c)] += vi;
	}
      }
    }
  }
  return fp;
}

/*! @brief  U_{x-\hat{\mu},\mu}^\dagger \psi_{x-\hat{\mu}} */

const Field Dirac_Wilson_Brillouin::sft_m(const Field& f,int dir)const{
  Field fm(fsize_);
  int Ni = 2*ND_*NC_; /*!< @brief num ob elements of a spinor */
  Format::Format_F ff = get_fermionFormat();

  /// boundary part ///
  int is = 0;
  int Xb = SiteIndex::instance()->Bdir(dir);
  int Nbdry = slsize(dir);
  double vbd[Ni*Nbdry]; /*!< @brief data on the upper boundary */
  for(int k=0; k<Nbdry; ++k){
    int xc = xsl(Xb,k,dir);
    const double* v = const_cast<Field&>(f).getaddr(ff_.index(0,xc));
    const double* U = const_cast<Field*>(u_)->getaddr(gf_.index(0,xc,dir));

    for(int s=0; s<ND_; ++s){    
      for(int c=0; c<NC_; ++c){
	vbd[sr(s,c)+is] = 0.0;  
	vbd[si(s,c)+is] = 0.0;
	
	for(int c1=0; c1<NC_; ++c1){
	  vbd[sr(s,c)+is] += U[re(c1,c)]*v[sr(s,c1)] +U[im(c1,c)]*v[si(s,c1)];
	  vbd[si(s,c)+is] -= U[im(c1,c)]*v[sr(s,c1)] -U[re(c1,c)]*v[si(s,c1)];
	}
      }
    }
    is += Ni;
  }
  double vbc[Ni*Nbdry];  //Copy vbd from backward processor
  comm_->transfer_bk(vbc,vbd,Ni*Nbdry,dir);
  is = 0;
  Nbdry = slsize(dir);
  
  for(int k=0; k<Nbdry; ++k){
    double* res = fm.getaddr(ff_.index(0,xsl(0,k,dir)));
    
    for(int s=0; s<ND_; ++s){  
      for(int c=0; c<NC_; ++c){  
	res[sr(s,c)] += vbc[sr(s,c)+is];  
	res[si(s,c)] += vbc[si(s,c)+is];
      }
    }
    is += Ni;
  }

  /// bulk part ///
  for(int x=1; x<Xb+1; ++x){
    int Nslice = slsize(dir);
    for(int k=0; k<Nslice; ++k){
      int xm = xsl(x-1,k,dir);
      const double* v = const_cast<Field&>(f).getaddr(ff_.index(0,xm));
      const double* U = const_cast<Field*>(u_)->getaddr(gf_.index(0,xm,dir));
      double* res = fm.getaddr(ff_.index(0,xsl(x,k,dir)));

      for(int s=0; s<ND_; ++s){
	for(int c=0; c<NC_; ++c){
	  double vr = 0.0, vi = 0.0; 
	
	  for(int c1=0; c1<NC_; ++c1){
	    vr += U[re(c1,c)]*v[sr(s,c1)] +U[im(c1,c)]*v[si(s,c1)];
	    vi -= U[im(c1,c)]*v[sr(s,c1)] -U[re(c1,c)]*v[si(s,c1)];
	  }
	  res[sr(s,c)] += vr;
	  res[si(s,c)] += vi;
	}
      }
    }
  }
  return fm;
}

const Field Dirac_Wilson_Brillouin::delg(const Field& f,int dir,double a,double b)const{
  using namespace FieldExpression;
  Field w = sft_p(f,dir);
  w -= sft_m(f,dir);
  w *= a;
  w += b*f;
  return (this->*gm[dir])(w);
}  

const Field Dirac_Wilson_Brillouin::lap(const Field& f,int dir,double a,double b)const{
  using  namespace FieldExpression;
  Format::Format_F ff = get_fermionFormat();
  Field w = sft_p(f,dir);
  w += sft_m(f,dir);
  w *= a;
  w += b*f;
  return w;
}  

const Field Dirac_Wilson_Brillouin::mult(const Field& f)const{
  using namespace FieldExpression;
  Format::Format_F ff = get_fermionFormat();
  Field w(fsize_);

  for(int mu=0; mu<NDIM_; ++mu){
    Field fn(fsize_),dn(fsize_);
    for(int nu=0; nu<NDIM_; ++nu){
      if(nu !=mu){
	Field fr(fsize_),dr(fsize_);
	for(int rho=0; rho<NDIM_; ++rho){
	  if(rho !=nu && rho != mu){
	    Field ft(fsize_),dt(fsize_);
	    int dir = sgm(mu,nu,rho);

	    ft += lap(f,dir,0.25,2.0);
	    dt += lap(f,dir,1.0/3,4.0);

	    fr += lap(ft,rho,1.0/3,0.0);
	    fr += (4.0/2)*f;
	    dr += lap(dt,rho,1.0/2,0.0);
	    dr += (16.0/2)*f;
	  }
	}
	fn += lap(fr,nu,0.5,0.0);
	fn += (8.0/3)*f;
	dn += lap(dr,nu,1.0,0.0);
	dn += (64.0/3)*f;
      }
    }
    w += delg(dn,mu,1.0/432,0.0);
    w -= 0.5*lap(fn,mu,1.0/64,0.0);
  }
  w *= kbr_;
  w += f;
  return w;
}

const Field Dirac_Wilson_Brillouin::gamma5(const Field& f) const{
  Field w(fsize_);
  int Nin = ff_.Nin();

  for(int site=0; site<Nvol_*Nin; site+=Nin){
    const double* ft = const_cast<Field&>(f).getaddr(site);
    double* res = w.getaddr(site);    

    for(int c=0; c<NC_; ++c){
      short int cc = 2*c;
      short int cc1 = 2*c+1;

      res[      cc] = ft[4*NC_+cc];  res[      cc1] = ft[4*NC_+cc1];
      res[2*NC_+cc] = ft[6*NC_+cc];  res[2*NC_+cc1] = ft[6*NC_+cc1];
      res[4*NC_+cc] = ft[      cc];  res[4*NC_+cc1] = ft[      cc1];
      res[6*NC_+cc] = ft[2*NC_+cc];  res[6*NC_+cc1] = ft[2*NC_+cc1];
    }
  }
  return w;
}

const Field Dirac_Wilson_Brillouin::gamma_x(const Field& f) const{
  Field w(fsize_);
  int Nin = ff_.Nin();

  for(int site=0; site<Nvol_*Nin; site+=Nin){
    const double* ft = const_cast<Field&>(f).getaddr(site);
    double* res = w.getaddr(site);    

    for(int c=0; c<NC_; ++c){
      short int cc = 2*c;
      short int cc1 = 2*c+1;
      
      res[      cc] = ft[6*NC_+cc1];  res[      cc1] =-ft[6*NC_+cc];
      res[2*NC_+cc] = ft[4*NC_+cc1];  res[2*NC_+cc1] =-ft[4*NC_+cc];
      res[4*NC_+cc] =-ft[2*NC_+cc1];  res[4*NC_+cc1] = ft[2*NC_+cc];
      res[6*NC_+cc] =-ft[      cc1];  res[6*NC_+cc1] = ft[      cc];
    }
  }
  return w;
}

const Field Dirac_Wilson_Brillouin::gamma_y(const Field& f) const{
  Field w(fsize_);
  int Nin = ff_.Nin();
  
  for(int site=0; site<Nvol_*Nin; site+=Nin){
    const double* ft = const_cast<Field&>(f).getaddr(site);
    double* res = w.getaddr(site);    

    for(int c=0; c<NC_; ++c){
      short int cc = 2*c;
      short int cc1 = 2*c+1;
      
      res[      cc] =-ft[6*NC_+cc];  res[      cc1] =-ft[6*NC_+cc1];
      res[2*NC_+cc] = ft[4*NC_+cc];  res[2*NC_+cc1] = ft[4*NC_+cc1];
      res[4*NC_+cc] = ft[2*NC_+cc];  res[4*NC_+cc1] = ft[2*NC_+cc1];
      res[6*NC_+cc] =-ft[      cc];  res[6*NC_+cc1] =-ft[      cc1];
    }
  }
  return w;
}

const Field Dirac_Wilson_Brillouin::gamma_z(const Field& f) const{
  Field w(fsize_);
  int Nin = ff_.Nin();

  for(int site=0; site<Nvol_*Nin; site+=Nin){
    const double* ft = const_cast<Field&>(f).getaddr(site);
    double* res = w.getaddr(site);    

    for(int c=0; c <NC_; ++c){
      short int cc = 2*c;
      short int cc1 = 2*c+1;
      
      res[      cc] = ft[4*NC_+cc1];  res[      cc1] =-ft[4*NC_+cc];
      res[2*NC_+cc] =-ft[6*NC_+cc1];  res[2*NC_+cc1] = ft[6*NC_+cc];
      res[4*NC_+cc] =-ft[      cc1];  res[4*NC_+cc1] = ft[      cc];
      res[6*NC_+cc] = ft[2*NC_+cc1];  res[6*NC_+cc1] =-ft[2*NC_+cc];
    }
  }
  return w;
}

const Field Dirac_Wilson_Brillouin::gamma_t(const Field& f) const{
  Field w(f);
  for(int site=0; site<Nvol_; ++site){
    w.set(ff_.cslice(2,site), -f[ff_.cslice(2,site)]);
    w.set(ff_.cslice(3,site), -f[ff_.cslice(3,site)]);
  }
  return w;
}
