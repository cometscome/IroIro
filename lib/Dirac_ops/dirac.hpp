/*!
 * @file dirac.hpp
 * @brief Defines the abstract base class for Dirac operators
 * Time-stamp: <2013-07-02 17:18:46 cossu>
 */
#ifndef DIRAC_H_
#define DIRAC_H_

#include "include/field.h"
#include "include/pugi_interface.h"

/*
 *! @class Dirac
 * @brief Declaration of abstract base class for Dirac operators
 */
class Dirac {
public:
  virtual ~Dirac(){}
  virtual size_t fsize() const = 0;
  virtual size_t gsize() const = 0;
  //  virtual int Nvol()const = 0;

  virtual const Field mult    (const Field&)const = 0;
  virtual const Field mult_dag(const Field&)const = 0;

  /////////////////////////////////Preconditioned versions
  virtual const Field mult_prec     (const Field&f)const{return f;}
  virtual const Field mult_dag_prec (const Field&f)const{return f;}
  virtual const Field left_prec     (const Field&f)const{return f;}
  virtual const Field right_prec    (const Field&f)const{return f;}
  virtual const Field left_dag_prec (const Field&f)const{return f;}
  virtual const Field right_dag_prec(const Field&f)const{return f;}
  ////////////////////////////////////////////

  virtual const Field md_force(const Field& eta,const Field& zeta)const{}
  virtual void update_internal_state(){}
};

#endif


