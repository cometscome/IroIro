/*! 
 * @file solver.hpp
 *
 * @brief Definition of abstract Solver class and SolverOutput structure
 *
 * Time-stamp: <2013-04-23 11:23:05 neo>
 */
#ifndef SOLVER_INCLUDED
#define SOLVER_INCLUDED

#include <string>
#include "include/macros.hpp" 

class Field;
class Fopr;

struct SolverOutput{
  double diff;   /*!< Residual */
  int Iterations; /*!< Iterations for convergence */
  double timing;
  std::string Msg;
  void print(std::string Msg2="");
};

class Solver{
public:
  virtual ~Solver(){}
  virtual SolverOutput solve(Field& solution, 
			     const Field& source) const =0;
  virtual bool check_DdagD() const  = 0;
};

#endif    
