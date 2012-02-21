//----------------------------------------------------------------------
// mdExec.hpp
//----------------------------------------------------------------------
#ifndef MDEXEC_INCLUDED
#define MDEXEC_INCLUDED
#include<vector>

#include "include/common_fields.hpp"

class Action;
class RandNum;
class Observer;

typedef std::vector<Action*> ActionLevel;
typedef std::vector<ActionLevel> ActionSet;
typedef std::vector<Observer*> ObserverList;

/*! 
 * @brief Abstract base class for Molecular Dynamics management
 *
 */
class MDexec{
private:
  virtual void update_P(int lv,double ep) = 0;
  virtual void update_U(double ep) = 0;

  virtual void register_observers() = 0;
  virtual void attach_observer(ObserverList&, Observer*) = 0;
  virtual void detach_observer(ObserverList&, Observer*) = 0;  
  virtual void notify_observers(ObserverList&) = 0;
  

public:
  virtual ~MDexec(){}
  virtual void init(std::vector<int>& clock,const GaugeField& U,
		    const RandNum& rand)=0;
  virtual double calc_H()const =0;
  virtual void integrator(int level,std::vector<int>& clock) =0;
  virtual const GaugeField get_U() const =0;
};

namespace MDutils{
  void md_mom(GaugeField& P,const RandNum& rand);
  void md_mom_su3(GaugeField& P,const RandNum& rand);
}
#endif//MDEXEC_INCLUDED
