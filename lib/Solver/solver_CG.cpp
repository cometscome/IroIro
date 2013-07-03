/*!
 * @file solver_CG.cpp
 * @brief Definition of Solver_CG class member functions
 *
 * Time-stamp: <2013-07-03 12:09:22 cossu>
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include "Communicator/comm_io.hpp"
#include "Fields/field_expressions.hpp"
#include "include/messages_macros.hpp"
#include "include/errors.hpp"
#include "solver_CG.hpp"

#ifdef IBM_BGQ_WILSON
#include "bgqwilson.h"
#endif

/*!
 * @brief Solves the linear equation (no preconditioning)
 *
 * @param xq  Output vector
 * @param b   Input vector
 */
SolverOutput Solver_CG::solve(Field& xq,const Field& b) const{ 

  _Message(SOLV_ITER_VERB_LEVEL, "CG solver start");

  SolverOutput Out;
  Out.Msg = "CG solver";
  Out.Iterations = -1;
  double kernel_timing = 0.0;

  TIMING_START;
  
  Field x = b;//initial condition
  Field r = b;//initial residual
  r -= opr_->mult(x);
  Field p = r;
  double rr = r*r;// (r,r)
  double snorm = b.norm();
  snorm = 1.0/(snorm*snorm);//need the square of the norm

  _Message(SOLV_ITER_VERB_LEVEL, " Snorm = "<< snorm<<"\n" );
  _Message(SOLV_ITER_VERB_LEVEL, " Init  = "<< rr*snorm<<"\n" );

  for(int it = 0; it < Params.MaxIter; ++it){
    solve_step(r,p,x,rr, kernel_timing);
    _Message(SOLV_ITER_VERB_LEVEL, "   "<< std::setw(5)<< "["<<it<<"] "
	     << std::setw(20) << rr*snorm<< "\n");
    if(rr*snorm < Params.GoalPrecision){
      Out.Iterations = it;
      break;
    }
  }

  if(Out.Iterations == -1) {
    std::ostringstream msg;
    msg << "CG solver not converged. Current residual: "<< rr*snorm;
    Errors::ConvergenceErr(msg);
  }

  p = opr_->mult(x);
  p -= b;
  Out.diff = p.norm()*sqrt(snorm);

  xq = x;

  TIMING_END(Out.timing);
  _Message(SOLV_ITER_VERB_LEVEL, 
	   "[Timing] Solver_CG Kernel section : "<< kernel_timing << "\n");

  return Out;
}

inline void Solver_CG::solve_step(Field& r,Field& p,Field& x,double& rr, 
				  double& opr_timing)const {
  using namespace FieldExpression;

  double* x_ptr = x.getaddr(0);
  double* p_ptr = p.getaddr(0);
  double* r_ptr = r.getaddr(0);

  timeval start, end;                
  gettimeofday(&start,NULL);

  Field s = opr_->mult(p);//Ap

  gettimeofday(&end,NULL);

  opr_timing += (end.tv_sec - start.tv_sec)*1000.0;
  opr_timing += (end.tv_usec - start.tv_usec) / 1000.0;   // us to ms

  double* s_ptr = s.getaddr(0);
#ifdef IBM_BGQ_WILSON
  ///////////////////////////////////////////////////
  int v_size = x.size()/24; // << assumes 24 elements
  double pap;
  BGWilsonLA_DotProd(&pap,p_ptr,s_ptr,v_size);
  pap = Communicator::instance()->reduce_sum(pap);
  double rrp = rr;
  double cr = rrp/pap;// (r,r)/(p,Ap)

  //  x += cr*p; // x = x + cr * p
  BGWilsonLA_MultAddScalar(x_ptr,p_ptr,cr,v_size);
  //  r -= cr*s; // r_k = r_k - cr * Ap
  BGWilsonLA_MultAddScalar(r_ptr,s_ptr,-cr,v_size);
  
  //  rr = r*r; // rr = (r_k,r_k)
  BGWilsonLA_Norm(&rr,r_ptr,v_size);
  rr = Communicator::instance()->reduce_sum(rr);
  //  p *= rr/rrp; // p = p*(r_k,r_k)/(r,r)
  //  p += r; // p = p + p*(r_k,r_k)/(r,r)
  BGWilsonLA_MultScalar_Add(p_ptr,r_ptr,rr/rrp,v_size);
  /////////////////////////////////////////////////////
#else
  double pap = p*s;// (p,Ap)
  double rrp = rr;
  register double cr = rrp/pap;// (r,r)/(p,Ap)
  rr = 0.0;

  for (int i = 0; i < x.size(); ++i){
    x_ptr[i] += cr * p_ptr[i];    // x = x + cr * p
    r_ptr[i] -= cr * s_ptr[i];    // r_k = r_k - cr * Ap
    rr += r_ptr[i]*r_ptr[i];      // rr = (r_k,r_k)
  }
  rr = Communicator::instance()->reduce_sum(rr);
  cr = rr/rrp;
  for(int i=0; i<p.size(); ++i)
    p_ptr[i] = p_ptr[i]*cr+r_ptr[i];
  
  //p *= rr/rrp; // p += p*(r_k,r_k)/(r,r)
  //p += r; // p = p + p*(r_k,r_k)/(r,r)
#endif
}

//////////////////////////////////////////////////////////////////////////////////
/*!
 * @brief Solves the linear equation (preconditioned version)
 *
 * @param xq  Output vector
 * @param b   Input vector
 */
SolverOutput Solver_CG_Precondition::solve(Field& xq, 
				   const Field& b) const{ 

#if VERBOSITY > 1
  CCIO::header("CG solver Preconditioned start");
#endif
  SolverOutput Out;
  Out.Msg = "CG solver Preconditioned";
  Out.Iterations = -1;

  TIMING_START;
  
  Field x = b;//starting guess
  Field r = b;
  r -= opr_->mult_prec(x);
  Field p = r;
  double rr = r*r;
  double snorm = b.norm();
  snorm = 1.0/(snorm*snorm);

#if VERBOSITY > 1
  CCIO::cout<<" Snorm = "<<snorm<<std::endl;
  CCIO::cout<<" Init  = "<< rr*snorm << std::endl;
#endif

  for(int it = 0; it < Params.MaxIter; it++){
    solve_step(r,p,x,rr);
#if VERBOSITY > 1
    CCIO::cout<< std::setw(5)<< "["<<it<<"] "<< std::setw(20) << rr*snorm<< "\n";
#endif    
  
    if(rr*snorm < Params.GoalPrecision){
      Out.Iterations = it;
      break;
    }
  }
  if(Out.Iterations == -1) throw "Not converged.";

  p = opr_->mult_prec(x);
  p -= b;
  Out.diff = p.norm()*sqrt(snorm);

  xq = x;

  TIMING_END(Out.timing);
  return Out;
}

inline void Solver_CG_Precondition::solve_step(Field& r,Field& p,Field& x,double& rr)const {
  
  using namespace FieldExpression;
  
  Field s = opr_->mult_prec(p);
  
  double pap = p*s;
  double rrp = rr;
  double cr = rrp/pap;
  
  Field v = p;
  x += cr*p;
  r -= cr*s;

  rr = r*r;
  p *= rr/rrp;
  p += r;
}

#ifdef IBM_BGQ_WILSON
SolverOutput Solver_CG_DWF_Optimized::solve(Field& xq,const Field& b) const{ 
  SolverOutput Out;
  opr_->solve_eo(xq, b, Out, Params.MaxIter,Params.GoalPrecision);
  return Out;
}
#endif
