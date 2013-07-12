/*!
 * @file dirac_BFM_wrapper.hpp
 * @brief Declares the wrapper classs for P. Boyle Bagel/BFM libs
 * Time-stamp: <2013-07-12 16:58:04 cossu>
 */
#ifndef DIRAC_BFM_WRAPPER_
#define DIRAC_BFM_WRAPPER_

//wrapper
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION
#include "bfm.h"
#include "Tools/Bagel/bfm_storage.hpp"
#include "Dirac_ops/dirac_WilsonLike.hpp"
#include "include/pugi_interface.h"

struct Dirac_BFM_Wrapper_params {
  std::string operator_name_;
  double mq_;
  double M5_;
  int Ls_;
  double scale_;

  //solver
  int max_iter_;
  double target_;

  Dirac_BFM_Wrapper_params(XML::node BFMnode);
  set_SolverParams(XML::node BFMnode);
};



/*
 *! @class Dirac_BFM_Wrapper
 * @brief Declaration of abstract base class for Dirac operators
 */
class Dirac_BFM_Wrapper: public DiracWilsonLike_EvenOdd {
private:
  Dirac_BFM_Wrapper_params BFMparams;
  bfmarg parameters;
  bfm_dp linop;
  unsigned int threads;
  BFM_Storage BFM_interface;
  std::string SolverName;

  //temporary hack - don't want this in the final version!!!!
  // supports methods like md_force
  DiracWilsonLike* Internal_EO;

  bool is_initialized;
  bool has_operator;
  bool has_solver_params;

  // Fields
  const Field* const u_;
  Fermion_t psi_h[2];//for sources e/o
  Fermion_t chi_h[2];//for results e/o
  Fermion_t tmp;

  int Nvol_;

  void set_ScaledShamirCayleyTanh(double mq, double M5, int Ls, double scale);

  void LoadSource(FermionField&, int);
  void LoadGuess(FermionField&, int);
  void GetSolution(FermionField&, int);
  void GetMultishiftSolutions(std::vector< FermionField >&,
			      std::vector< Fermion_t >&,
			      int);
  Dirac_BFM_Wrapper(); //hides default constructor
  void AllocateFields();
public:
  Dirac_BFM_Wrapper(XML::node, const Field*, DiracWilsonLike*);
  ~Dirac_BFM_Wrapper();
  size_t fsize() const{};
  size_t gsize() const{};

  void initialize();
  // Slow implementation: don't use this, use the solver
  // here just for compatibility
  const Field mult    (const Field&)const;
  Field mult_test    (const Field&);
  const Field mult_dag(const Field&)const;

  const Field gamma5(const Field&) const{};
  const Field md_force(const Field& eta,const Field& zeta)const;
  void update_internal_state(){};

  // Even/Odd part - now dummy, not needed
  const Field mult_eo(const Field&) const{};
  const Field mult_oe(const Field&) const{};
  const Field mult_eo_dag(const Field&) const{};
  const Field mult_oe_dag(const Field&) const{};
  
  const Field mult_ee(const Field&) const{};
  const Field mult_oo(const Field&) const{};
  const Field mult_ee_inv(const Field&) const{};
  const Field mult_oo_inv(const Field&) const{};
  
  const DiracWilsonLike* getDeo() const{};
  const DiracWilsonLike* getDoe() const{};
  

  // Solvers wrapper
  void solve_CGNE(FermionField& out, FermionField& in);
  void set_SolverParams(XML::node);
  void solve_CGNE_multishift(std::vector < FermionField >& solution, 
			     FermionField& source,
			     vector_double shifts,
			     vector_double mresiduals);

};

#endif
