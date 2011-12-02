/*! 
 * @file action_Nf2_DomainWall.h
 *
 * @brief Declaration of Action_Nf2_DomainWall class
 *
 */
#ifndef ACTION_NF2_DOMAINWALL_INCLUDED
#define ACTION_NF2_DOMAINWALL_INCLUDED

#include "Tools/randNum_MP.h"
#include "Action/action.h"
#include "include/fopr.h"
#include "include/format_G.h"
#include "Dirac_ops/dirac_Operator_Factory.hpp"
#include "Solver/solver_Factory.hpp"
#include "Dirac_ops/dirac_DomainWall.hpp"

/*!
 * @class Action_Nf2_DomainWall
 *
 * @brief Class to calculate TwoFlavors HMC action term for DWF
 *
 */
class Action_Nf2_DomainWall :public Action{
private:
  Field* const u_;    /*!< The gauge field       */
  const Dirac* D_;    /*!< Dirac Kernel operator */ 
  const Solver* slv_; /*!< Linear solver operator*/

  Action_Nf2_ratio action_;
  Dirac_optimalDomainWall Dpv_;

  Field phi_;
  size_t fsize_;
  
  Field DdagD_inv(const Field& src);
  
public:
  /*!
   * @brief Standard constructor 
   */
  Action_Nf2_DomainWall(Field* const GField,
			const Dirac_optimalDomainWall* D, 
			const Solver* Solv)
    :u_(GField),
     Dpv_(*D,DomainWallFermions::PauliVillars()),
     action_(GField,D,&Dpv_,Solv,Solv),
     fsize_(D->fsize()){}
  
  ~Action_Nf2_DomainWall(){}

  void init(Field& F,const RandNum& rand,const void* vp= 0){
    action_.init(F,rand,vp);
  }

  void init(Field& P,const RandNum& rand,const Field& U,const void* vp= 0){
    action_.init(P,rand,U,vp);
  }

  double calc_H(){return action_.calc_H();}

  Field md_force(const void* vp = 0){ return action_.md_force(vp);}
  Field md_force(const Field& U, const void* vp = 0){ 
    return action_.md_force(U,vp);
  }
};
#endif
