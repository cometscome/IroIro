//---------------------------------------------------------------------
/*! @file sunMatUtils.hpp
  @brief \f$SU(N)\f$ Matrices linear algebra Utilities
*/ 
//---------------------------------------------------------------------
#ifndef SUNMATUTILS_H_
#define SUNMATUTILS_H_

#include "sunMat.hpp"
#include "include/common_fields.hpp"

class RandNum;

namespace SUNmatUtils{
  const SUNmat unity();
  const SUNmat zero();
  double ReTr(const SUNmat&);  
  double ImTr(const SUNmat&);
  const SUNmat dag(const SUNmat&);  
  const SUNmat xI(const SUNmat&);  
  const SUNmat operator+(const SUNmat&,const SUNmat&);
  const SUNmat operator-(const SUNmat&,const SUNmat&);
  const SUNmat operator*(const SUNmat&,const SUNmat&);
  const SUNmat operator*(const SUNmat&,double);
  const SUNmat operator/(const SUNmat&,double);
  const SUNmat reunit(const SUNmat&);
  const std::valarray<double> trace_less(const SUNmat&);
  const SUNmat anti_hermite_traceless(const SUNmat&);
  const SUNmat anti_hermite(const SUNmat&);
  const SUNmat outer_prod(const SUNvec& v,const SUNvec& w);
  const SUNmat random_mat(const RandNum& rand);
  const SUNmat exponential(const SUNmat& X,int N,int n=1);
  /*  
  template<size_t COLORS>
  double ReTr(const SUNmatrix<COLORS>& m){
    double tr = 0.0;
    for(int c=0; c<COLORS; ++c) tr += m.r(c,c);
    return tr;
  }

  template<size_t COLORS>
  double ImTr(const SUNmatrix<COLORS>& m){
    double tr = 0.0;
    for(int c=0; c<COLORS; ++c) tr += m.i(c,c);
    return tr;
  }
  */
  // multiplication of the Gell-Mann matrices
  const SU3mat lambda1(const SU3mat&);
  const SU3mat lambda2(const SU3mat&);
  const SU3mat lambda3(const SU3mat&);
  const SU3mat lambda4(const SU3mat&);
  const SU3mat lambda5(const SU3mat&);
  const SU3mat lambda6(const SU3mat&);
  const SU3mat lambda7(const SU3mat&);
  const SU3mat lambda8(const SU3mat&);
  
  const SU3mat (*const lambda[])(const SU3mat&) = {
    lambda1,lambda2,lambda3,lambda4,lambda5,lambda6,lambda7,lambda8 };

  // obtain the adjoint representation
  template<size_t COLORS>
  const std::valarray<double> adjoint(const SUNmatrix<COLORS>& u){
    CCIO::cout<<"implemented only for COLOR=3"<<std::endl;
    abort();
  }

  template<>
  const std::valarray<double> adjoint(const SUNmatrix<3>& u);

  //BLAS style multiplications for optimization purposes
  // Matrix-matrix
  const SUNmat gemm(const SUNmat&, const SUNmat&);
  // Matrix-matrix^dag)
  const SUNmat gemmd(const SUNmat&, const SUNmat&);
  // (Matrix^dag)-matrix
  const SUNmat gemdm(const SUNmat&, const SUNmat&);
  //////////////////////////////////////////////////////

  void SUNprint(const SUNmat&);

}//endof namespace SUNmat_utils

#endif
