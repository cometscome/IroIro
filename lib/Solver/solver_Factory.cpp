/*!
 * @file solver_Factory.cpp 
 * @brief Definition of Solver operators factories
 * Time-stamp: <2013-07-06 11:11:41 noaki>
 */
#include "solver_Factory.hpp"
#include <string.h>

namespace Solvers {
  SolverFactory* createSolverFactory(const XML::node node){
    if(node !=NULL) {
      const char* Solver_name = node.attribute("type").value();
      
      if(!strcmp(Solver_name, "Solver_CG")) 
	return new SolverCGFactory(node);
      if(!strcmp(Solver_name, "Solver_BiCGStab")) 
	return new SolverBiCGStabFactory(node);
      std::cerr << "No Solver available with type ["
		<< Solver_name << "]\n" 
		<< "Request by " << node.parent().name() << std::endl;
      abort();
    }else {
      std::cout << "Requested node is missing in input file (Solver Object)\n"
		<< "Request by " << node.parent().name() << std::endl;
      abort();
    }
  }

  RationalSolverFactory* createRationalSolverFactory(const XML::node node){  
    if(node !=NULL) {
      const char* Solver_name = node.attribute("type").value();
      
      if(!strcmp(Solver_name, "Rational_CG")) 
	return new RationalSolverCGFactory(node);

      std::cerr << "No Solver available with type ["
		<< Solver_name << "]\n" 
		<< "Request by " << node.parent().name() << std::endl;
      abort();
    }else {
      std::cout << "Requested node is missing in input file (RationalSolver Object)\n"
		<< "Request by " << node.parent().name() << std::endl;
      abort();
    }
  }

}//end of namespace
