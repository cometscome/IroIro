//----------------------------------------------------------------------
// dirac_wilson.cpp
//----------------------------------------------------------------------
#include "dirac_wilson.h"

//using namespace SUNvec_utils;
using namespace std;

#if 1
void Dirac_Wilson::mult_xp(Field& fp, ShiftField* sfp) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site <Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c1][c2][0] = (*u_)[gf_->index_r(c1,c2,site,0)];
	utmp[c1][c2][1] = (*u_)[gf_->index_i(c1,c2,site,0)];
      }
    }
    bool on = sfp->on_bdry(site);
    double v1tmp[Nc][2], v2tmp[Nc][2];
    if (on) {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bdry(c,0,site) - sfp->im_on_bdry(c,3,site);
	v1tmp[c][1] = sfp->im_on_bdry(c,0,site) + sfp->re_on_bdry(c,3,site);
	v2tmp[c][0] = sfp->re_on_bdry(c,1,site) - sfp->im_on_bdry(c,2,site);
	v2tmp[c][1] = sfp->im_on_bdry(c,1,site) + sfp->re_on_bdry(c,2,site);
      } 
    } else {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bulk(c,0,site) - sfp->im_on_bulk(c,3,site);
	v1tmp[c][1] = sfp->im_on_bulk(c,0,site) + sfp->re_on_bulk(c,3,site);
	v2tmp[c][0] = sfp->re_on_bulk(c,1,site) - sfp->im_on_bulk(c,2,site);
	v2tmp[c][1] = sfp->im_on_bulk(c,1,site) + sfp->re_on_bulk(c,2,site);
      }
    }

    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
		   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      fp.add(ff_->index_r(c,0,site),  v1[c][0]);
      fp.add(ff_->index_i(c,0,site),  v1[c][1]);
      fp.add(ff_->index_r(c,1,site),  v2[c][0]);
      fp.add(ff_->index_i(c,1,site),  v2[c][1]);
      fp.add(ff_->index_r(c,2,site),  v2[c][1]);
      fp.add(ff_->index_i(c,2,site), -v2[c][0]);
      fp.add(ff_->index_r(c,3,site),  v1[c][1]);
      fp.add(ff_->index_i(c,3,site), -v1[c][0]);
    }
  }
}

void Dirac_Wilson::mult_yp(Field& fp, ShiftField* sfp) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site <Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c1][c2][0] = (*u_)[gf_->index_r(c1,c2,site,1)];
	utmp[c1][c2][1] = (*u_)[gf_->index_i(c1,c2,site,1)];
      }
    }

    bool on = sfp->on_bdry(site);
    double v1tmp[Nc][2], v2tmp[Nc][2];
    if (on) {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bdry(c,0,site) + sfp->re_on_bdry(c,3,site);
	v1tmp[c][1] = sfp->im_on_bdry(c,0,site) + sfp->im_on_bdry(c,3,site);
	v2tmp[c][0] = sfp->re_on_bdry(c,1,site) - sfp->re_on_bdry(c,2,site);
	v2tmp[c][1] = sfp->im_on_bdry(c,1,site) - sfp->im_on_bdry(c,2,site);
      }
    } else { 
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bulk(c,0,site) + sfp->re_on_bulk(c,3,site);
	v1tmp[c][1] = sfp->im_on_bulk(c,0,site) + sfp->im_on_bulk(c,3,site);
	v2tmp[c][0] = sfp->re_on_bulk(c,1,site) - sfp->re_on_bulk(c,2,site);
	v2tmp[c][1] = sfp->im_on_bulk(c,1,site) - sfp->im_on_bulk(c,2,site);
      }
    }
    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      fp.add(ff_->index_r(c,0,site),  v1[c][0]);
      fp.add(ff_->index_i(c,0,site),  v1[c][1]);
      fp.add(ff_->index_r(c,1,site),  v2[c][0]);
      fp.add(ff_->index_i(c,1,site),  v2[c][1]);
      fp.add(ff_->index_r(c,2,site), -v2[c][0]);
      fp.add(ff_->index_i(c,2,site), -v2[c][1]);
      fp.add(ff_->index_r(c,3,site),  v1[c][0]);
      fp.add(ff_->index_i(c,3,site),  v1[c][1]);
    }
  }
}

void Dirac_Wilson::mult_zp(Field& fp, ShiftField* sfp) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site <Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c1][c2][0] = (*u_)[gf_->index_r(c1,c2,site,2)];
	utmp[c1][c2][1] = (*u_)[gf_->index_i(c1,c2,site,2)];
      }
    }
    bool on = sfp->on_bdry(site);
    double v1tmp[Nc][2], v2tmp[Nc][2];
    if (on) {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bdry(c,0,site) - sfp->im_on_bdry(c,2,site);
	v1tmp[c][1] = sfp->im_on_bdry(c,0,site) + sfp->re_on_bdry(c,2,site);
	v2tmp[c][0] = sfp->re_on_bdry(c,1,site) + sfp->im_on_bdry(c,3,site);
	v2tmp[c][1] = sfp->im_on_bdry(c,1,site) - sfp->re_on_bdry(c,3,site);
      }
    } else {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bulk(c,0,site) - sfp->im_on_bulk(c,2,site);
	v1tmp[c][1] = sfp->im_on_bulk(c,0,site) + sfp->re_on_bulk(c,2,site);
	v2tmp[c][0] = sfp->re_on_bulk(c,1,site) + sfp->im_on_bulk(c,3,site);
	v2tmp[c][1] = sfp->im_on_bulk(c,1,site) - sfp->re_on_bulk(c,3,site);
      }
    }

    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }

    for (int c = 0; c < Nc; ++c) {
      fp.add(ff_->index_r(c,0,site),  v1[c][0]);
      fp.add(ff_->index_i(c,0,site),  v1[c][1]);
      fp.add(ff_->index_r(c,1,site),  v2[c][0]);
      fp.add(ff_->index_i(c,1,site),  v2[c][1]);
      fp.add(ff_->index_r(c,2,site),  v1[c][1]);
      fp.add(ff_->index_i(c,2,site), -v1[c][0]);
      fp.add(ff_->index_r(c,3,site), -v2[c][1]);
      fp.add(ff_->index_i(c,3,site),  v2[c][0]);
    }
  }
}

void Dirac_Wilson::mult_tp(Field& fp, ShiftField* sfp) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site <Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c1][c2][0] = (*u_)[gf_->index_r(c1,c2,site,3)];
	utmp[c1][c2][1] = (*u_)[gf_->index_i(c1,c2,site,3)];
      }
    }

    bool on = sfp->on_bdry(site);
    double v1tmp[Nc][2], v2tmp[Nc][2];
    if (on) {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bdry(c,2,site)*2.0;
	v1tmp[c][1] = sfp->im_on_bdry(c,2,site)*2.0;
	v2tmp[c][0] = sfp->re_on_bdry(c,3,site)*2.0;
	v2tmp[c][1] = sfp->im_on_bdry(c,3,site)*2.0;
      }
    } else {
      for (int c = 0; c < Nc; ++c) {
	v1tmp[c][0] = sfp->re_on_bulk(c,2,site)*2.0;
	v1tmp[c][1] = sfp->im_on_bulk(c,2,site)*2.0;
	v2tmp[c][0] = sfp->re_on_bulk(c,3,site)*2.0;
	v2tmp[c][1] = sfp->im_on_bulk(c,3,site)*2.0;
      }
    }
    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      fp.add(ff_->index_r(c,2,site),  v1[c][0]);
      fp.add(ff_->index_i(c,2,site),  v1[c][1]);
      fp.add(ff_->index_r(c,3,site),  v2[c][0]);
      fp.add(ff_->index_i(c,3,site),  v2[c][1]);
    }
  }
}

void Dirac_Wilson::mult_xm(valarray<double>& w, const Field& f) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site <Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c2][c1][0] =   (*u_)[gf_->index_r(c1,c2,site,0)];
	utmp[c2][c1][1] = - (*u_)[gf_->index_i(c1,c2,site,0)];
      }
    }
    double v1tmp[Nc][2], v2tmp[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1tmp[c][0] = f[ff_->index_r(c,0,site)] + f[ff_->index_i(c,3,site)];
      v1tmp[c][1] = f[ff_->index_i(c,0,site)] - f[ff_->index_r(c,3,site)];
      v2tmp[c][0] = f[ff_->index_r(c,1,site)] + f[ff_->index_i(c,2,site)];
      v2tmp[c][1] = f[ff_->index_i(c,1,site)] - f[ff_->index_r(c,2,site)];
    }
    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      w[ff_->index_r(c,0,site)] =  v1[c][0];
      w[ff_->index_i(c,0,site)] =  v1[c][1];
      w[ff_->index_r(c,1,site)] =  v2[c][0];
      w[ff_->index_i(c,1,site)] =  v2[c][1];
      w[ff_->index_r(c,2,site)] = -v2[c][1];
      w[ff_->index_i(c,2,site)] =  v2[c][0];
      w[ff_->index_r(c,3,site)] = -v1[c][1];
      w[ff_->index_i(c,3,site)] =  v1[c][0];
    }
  }
}

void Dirac_Wilson::mult_ym(valarray<double>& w, const Field& f) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site <Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c2][c1][0] =   (*u_)[gf_->index_r(c1,c2,site,1)];
	utmp[c2][c1][1] = - (*u_)[gf_->index_i(c1,c2,site,1)];
      }
    }
    double v1tmp[Nc][2], v2tmp[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1tmp[c][0] = f[ff_->index_r(c,0,site)] - f[ff_->index_r(c,3,site)];
      v1tmp[c][1] = f[ff_->index_i(c,0,site)] - f[ff_->index_i(c,3,site)];
      v2tmp[c][0] = f[ff_->index_r(c,1,site)] + f[ff_->index_r(c,2,site)];
      v2tmp[c][1] = f[ff_->index_i(c,1,site)] + f[ff_->index_i(c,2,site)];
    }
    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      w[ff_->index_r(c,0,site)] =  v1[c][0];
      w[ff_->index_i(c,0,site)] =  v1[c][1];
      w[ff_->index_r(c,1,site)] =  v2[c][0];
      w[ff_->index_i(c,1,site)] =  v2[c][1];
      w[ff_->index_r(c,2,site)] =  v2[c][0];
      w[ff_->index_i(c,2,site)] =  v2[c][1];
      w[ff_->index_r(c,3,site)] = -v1[c][0];
      w[ff_->index_i(c,3,site)] = -v1[c][1];
    }
  }
}

void Dirac_Wilson::mult_zm(valarray<double>& w, const Field& f) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site < Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c2][c1][0] =   (*u_)[gf_->index_r(c1,c2,site,2)];
	utmp[c2][c1][1] = - (*u_)[gf_->index_i(c1,c2,site,2)];
      }
    }
    double v1tmp[Nc][2], v2tmp[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1tmp[c][0] = f[ff_->index_r(c,0,site)] + f[ff_->index_i(c,2,site)];
      v1tmp[c][1] = f[ff_->index_i(c,0,site)] - f[ff_->index_r(c,2,site)];
      v2tmp[c][0] = f[ff_->index_r(c,1,site)] - f[ff_->index_i(c,3,site)];
      v2tmp[c][1] = f[ff_->index_i(c,1,site)] + f[ff_->index_r(c,3,site)];
    }
    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      w[ff_->index_r(c,0,site)] =  v1[c][0];
      w[ff_->index_i(c,0,site)] =  v1[c][1];
      w[ff_->index_r(c,1,site)] =  v2[c][0];
      w[ff_->index_i(c,1,site)] =  v2[c][1];
      w[ff_->index_r(c,2,site)] = -v1[c][1];
      w[ff_->index_i(c,2,site)] =  v1[c][0];
      w[ff_->index_r(c,3,site)] =  v2[c][1];
      w[ff_->index_i(c,3,site)] = -v2[c][0];
    }
  }
}

void Dirac_Wilson::mult_tm(valarray<double>& w, const Field& f) const{
  int Nc = CommonPrms::instance()->Nc();

  for(int site = 0; site < Nvol_; ++site){
    double utmp[Nc][Nc][2];
    for (int c1 = 0; c1 < Nc; ++c1) {
      for (int c2 = 0; c2 < Nc; ++c2) {
	utmp[c2][c1][0] =   (*u_)[gf_->index_r(c1,c2,site,3)];
	utmp[c2][c1][1] = - (*u_)[gf_->index_i(c1,c2,site,3)];
      }
    }
    double v1tmp[Nc][2], v2tmp[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1tmp[c][0] = f[ff_->index_r(c,0,site)]*2.0;
      v1tmp[c][1] = f[ff_->index_i(c,0,site)]*2.0;
      v2tmp[c][0] = f[ff_->index_r(c,1,site)]*2.0;
      v2tmp[c][1] = f[ff_->index_i(c,1,site)]*2.0;
    }
    double v1[Nc][2], v2[Nc][2];
    for (int c = 0; c < Nc; ++c) {
      v1[c][0] = 0.0; v1[c][1] = 0.0; v2[c][0] = 0.0; v2[c][1] = 0.0;
      for (int c1 = 0; c1 < Nc; ++c1) {
	v1[c][0] += (utmp[c][c1][0]*v1tmp[c1][0] 
                   - utmp[c][c1][1]*v1tmp[c1][1]);
	v1[c][1] += (utmp[c][c1][1]*v1tmp[c1][0] 
                   + utmp[c][c1][0]*v1tmp[c1][1]);
	v2[c][0] += (utmp[c][c1][0]*v2tmp[c1][0] 
                   - utmp[c][c1][1]*v2tmp[c1][1]);
	v2[c][1] += (utmp[c][c1][1]*v2tmp[c1][0] 
                   + utmp[c][c1][0]*v2tmp[c1][1]);
      }
    }
    for (int c = 0; c < Nc; ++c) {
      w[ff_->index_r(c,0,site)] =  v1[c][0];
      w[ff_->index_i(c,0,site)] =  v1[c][1];
      w[ff_->index_r(c,1,site)] =  v2[c][0];
      w[ff_->index_i(c,1,site)] =  v2[c][1];
      w[ff_->index_r(c,2,site)] =  0.0;
      w[ff_->index_i(c,2,site)] =  0.0;
      w[ff_->index_r(c,3,site)] =  0.0;
      w[ff_->index_i(c,3,site)] =  0.0;
    }
  }
}
#endif  /*1*/

#if 0
void Dirac_Wilson::mult_xp(Field& fp, ShiftField* sfp) const{
  for(int site = 0; site <Nvol_; ++site){
    SUNvec v1 = u(site,0)*(v(sfp,0,site) +v_Ix(sfp,3,site));
    SUNvec v2 = u(site,0)*(v(sfp,1,site) +v_Ix(sfp,2,site));

    fp.add(ff_->cslice(0,site), v1.getva()); 
    fp.add(ff_->cslice(1,site), v2.getva()); 
    fp.add(ff_->cslice(2,site),-v2.xI().getva());
    fp.add(ff_->cslice(3,site),-v1.xI().getva());
  }
}

void Dirac_Wilson::mult_yp(Field& fp, ShiftField* sfp) const{
  for(int site = 0; site <Nvol_; ++site){
    SUNvec v1 = u(site,1)*(v(sfp,0,site) +v(sfp,3,site));
    SUNvec v2 = u(site,1)*(v(sfp,1,site) -v(sfp,2,site));

    fp.add(ff_->cslice(0,site), v1.getva()); 
    fp.add(ff_->cslice(1,site), v2.getva()); 
    fp.add(ff_->cslice(2,site),-v2.getva()); 
    fp.add(ff_->cslice(3,site), v1.getva()); 
  }
}

void Dirac_Wilson::mult_zp(Field& fp, ShiftField* sfp) const{
  for(int site = 0; site <Nvol_; ++site){
    SUNvec v1 = u(site,2)*(v(sfp,0,site) +v_Ix(sfp,2,site));
    SUNvec v2 = u(site,2)*(v(sfp,1,site) -v_Ix(sfp,3,site));

    fp.add(ff_->cslice(0,site), v1.getva());
    fp.add(ff_->cslice(1,site), v2.getva());
    fp.add(ff_->cslice(2,site),-v1.xI().getva());
    fp.add(ff_->cslice(3,site), v2.xI().getva());
  }
}

void Dirac_Wilson::mult_tp(Field& fp, ShiftField* sfp) const{
  for(int site = 0; site <Nvol_; ++site){
    SUNvec v1 = u(site,3)*v(sfp,2,site)*2.0;
    SUNvec v2 = u(site,3)*v(sfp,3,site)*2.0;

    fp.add(ff_->cslice(2,site), v1.getva());
    fp.add(ff_->cslice(3,site), v2.getva());
  }
}

void Dirac_Wilson::mult_xm(valarray<double>& w, const Field& f) const{
  w =0.0;
  for(int site = 0; site <Nvol_; ++site){
    SUNvec v1 = u_dag(site,0)*(v(f,0,site) -v_Ix(f,3,site));
    SUNvec v2 = u_dag(site,0)*(v(f,1,site) -v_Ix(f,2,site));

    w[ff_->cslice(0,site)] = v1.getva();
    w[ff_->cslice(1,site)] = v2.getva();
    w[ff_->cslice(2,site)] = v2.xI().getva();
    w[ff_->cslice(3,site)] = v1.xI().getva();
  }
}

void Dirac_Wilson::mult_ym(valarray<double>& w, const Field& f) const{
  w =0.0;
  for(int site = 0; site <Nvol_; ++site){
    SUNvec v1 = u_dag(site,1)*(v(f,0,site) -v(f,3,site));
    SUNvec v2 = u_dag(site,1)*(v(f,1,site) +v(f,2,site));
    
    w[ff_->cslice(0,site)] = v1.getva();
    w[ff_->cslice(1,site)] = v2.getva();
    w[ff_->cslice(2,site)] = v2.getva();
    w[ff_->cslice(3,site)] =-v1.getva(); 
  }
}

void Dirac_Wilson::mult_zm(valarray<double>& w, const Field& f) const{
  w =0.0;
  for(int site = 0; site < Nvol_; ++site){
    SUNvec v1 = u_dag(site,2)*(v(f,0,site) -v_Ix(f,2,site));
    SUNvec v2 = u_dag(site,2)*(v(f,1,site) +v_Ix(f,3,site));
    
    w[ff_->cslice(0,site)] = v1.getva();
    w[ff_->cslice(1,site)] = v2.getva();
    w[ff_->cslice(2,site)] = v1.xI().getva(); 
    w[ff_->cslice(3,site)] =-v2.xI().getva();
  }
}

void Dirac_Wilson::mult_tm(valarray<double>& w, const Field& f) const{
  w =0.0;
  for(int site = 0; site < Nvol_; ++site){
    SUNvec v1 = u_dag(site,3)*v(f,0,site)*2.0;
    SUNvec v2 = u_dag(site,3)*v(f,1,site)*2.0;
    
    w[ff_->cslice(0,site)] = v1.getva();
    w[ff_->cslice(1,site)] = v2.getva();
  }
}
#endif /*0*/

void (Dirac_Wilson::*Dirac_Wilson::mult_p[])
(Field&,ShiftField*) const = {&Dirac_Wilson::mult_xp,
			      &Dirac_Wilson::mult_yp,
			      &Dirac_Wilson::mult_zp,
			      &Dirac_Wilson::mult_tp,};

void (Dirac_Wilson::*Dirac_Wilson::mult_m[])
(valarray<double>&,const Field&) const = {&Dirac_Wilson::mult_xm,
					  &Dirac_Wilson::mult_ym,
					  &Dirac_Wilson::mult_zm,
					  &Dirac_Wilson::mult_tm,};

const Field Dirac_Wilson::gamma5(const Field& f) const{
  Field w(fsize_);
  for(int site = 0; site<Nvol_; ++site){
    w.set(ff_->cslice(0,site),f[ff_->cslice(2,site)]);
    w.set(ff_->cslice(1,site),f[ff_->cslice(3,site)]);
    w.set(ff_->cslice(2,site),f[ff_->cslice(0,site)]);
    w.set(ff_->cslice(3,site),f[ff_->cslice(1,site)]);
  }
  return w;
}

void Dirac_Wilson::mult_core(Field& w, const Field& f) const{
  valarray<double> wt(fsize_);

  for(int d=0; d <Ndim_; ++d){
    sf_up_->setup(f,d);
    (this->*mult_p[d])(w,sf_up_);

    (this->*mult_m[d])(wt,f);
    sf_dn_->setup(wt,d);
    w += sf_dn_->field();
  }
  w *= -Params.kappa;
}

const Field Dirac_Wilson::mult(const Field& f) const{
  Field w(fsize_);
  mult_core(w,f);
  w += f;
  return w;
}

const Field Dirac_Wilson::mult_dag(const Field& f)const{ 
  return gamma5(mult(gamma5(f)));
}

const Field Dirac_Wilson::md_force(const Field& eta,const Field& zeta)const{
  using namespace SUNmat_utils;

  int Nc = CommonPrms::instance()->Nc();
  int Nd = CommonPrms::instance()->Nd();

  Field et5 = gamma5(eta);
  Field zt5 = gamma5(zeta);

  Field xi(fsize_), xi5(fsize_);
  Field fce(gf_->size());

  SUNmat f;
  for(int mu=0; mu<Ndim_; ++mu){
    xi = 0.0;
    xi5 = 0.0;

    sf_up_->setup(eta,mu);
    (this->*mult_p[mu])(xi, sf_up_);

    sf_up_->setup(zt5,mu);
    (this->*mult_p[mu])(xi5,sf_up_);

    for(int site=0; site<Nvol_; ++site){
      for(int a=0; a<Nc; ++a){
        for(int b=0; b<Nc; ++b){
          double fre = 0.0;
          double fim = 0.0;
          for(int s=0; s<Nd; ++s){

	    size_t ra =ff_->index_r(a,s,site);
	    size_t ia =ff_->index_i(a,s,site);

	    size_t rb =ff_->index_r(b,s,site);
	    size_t ib =ff_->index_i(b,s,site);

	    fre += xi[ra]*zeta[rb] +xi[ ia]*zeta[ib]
	         -et5[ra]*xi5[ rb] -et5[ia]*xi5[ ib];
            fim +=-xi[ra]*zeta[ib] +xi[ ia]*zeta[rb]
	         +et5[ra]*xi5[ ib] -et5[ia]*xi5[ rb];
          }
          f.set(a,b,fre,fim);
        }
      }
      fce.set(gf_->cslice(0,site,mu),anti_hermite(f));
    }
  }
  fce *= -2*Params.kappa;
  return fce;
}

