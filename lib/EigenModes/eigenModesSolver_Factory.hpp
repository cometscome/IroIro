/*! @file eigenModsSolver_Factory.hpp
 *  @brief factory of EigenmodesSolver
 */
#ifndef EIGENMODESSOLVER_FACTORY_INCLUDED
#define EIGENMODESSOLVER_FACTORY_INCLUDED

#include "include/common_fields.hpp"
#include "eigenModesSolver_IRL.hpp"
#include "eigenSorter.hpp"
#include "include/foprHermFactory.hpp"
#include "include/pugi_interface.h"
#include <memory>

class EigenSolverFactory{
public:
  virtual EigenModesSolver* getEigenSolver(Fopr_Herm*,EigenSorter*)const =0;
  virtual ~EigenSolverFactory(){}
};

class EigenSolverFactory_IRL: public EigenSolverFactory{
  const XML::node eigslv_node_;
  mutable std::auto_ptr<Fopr_Herm> opr_ptr_;
  mutable std::auto_ptr<EigenSorter> str_ptr_;
public:
  EigenSolverFactory_IRL(XML::node node):eigslv_node_(node){}
  
  EigenModesSolver* getEigenSolver(Fopr_Herm* opr,EigenSorter* str)const{
    opr_ptr_.reset(opr);
    str_ptr_.reset(str);
    return new EigenModesSolver_IRL(opr_ptr_.get(),str_ptr_.get(),eigslv_node_);
  }
};

namespace EigenModes{
  EigenSolverFactory* createEigenSolverFactory(XML::node);
}

#endif
