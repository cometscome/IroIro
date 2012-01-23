/*!
  @file SmartConf.hpp

  @brief Declares the SmartConf class
*/


#ifndef SMART_CONF_H_
#define SMART_CONF_H_

#include <complex>
#include "StoutSmear.hpp"

class Field;
class Action;
/*!

  @brief Smeared configuration container

  It will behave like a configuration from the point of view of
  the HMC update and integrators.
  An "advanced configuration" object that can provide not only the 
  data to store the gauge configuration but also operations to manipulate
  it like smearing.

  It stores a list of smeared configurations.
*/
class SmartConf {
  // Private members
  const int smearingLevels;
  Smear_Stout StoutSmearing;
  std::vector<GaugeField> SmearedSet;
  Field* CurrentConfPtr; /*! @brief Pointer to the current 
			   smeared configuration */
  
  // Member functions
  void fill_smearedSet();
  GaugeField AnalyticSmearedForce(const GaugeField&, 
				   const Field&) const;
  const Field& get_smeared_conf(int) const;

  void set_iLambda(GaugeField& iLambda, 
		   GaugeField& e_iQ,
                   const GaugeField& iQ, 
		   const GaugeField& Sigmap,
                   const Field& U)const;
  
  void set_uw(double& u, double& w,
              const SUNmat& iQ1, const SUNmat& iQ2)const ;
  void set_fj(std::complex<double>& f0, std::complex<double>& f1,
              std::complex<double>& f2, const double& u,
	      const double& w)const;
  
  double func_xi0(double w)const;
  double func_xi1(double w)const;

public:
  Field* ThinLinks;      /*!< @brief Pointer to the thin 
			   links configuration */
  /*! @brief XML constructor */
  //SmartConf(XML::node node, Field* Config, Smear_Stout& Stout)

  /*! @brief Standard constructor */
  SmartConf(int Nsmear, Smear_Stout& Stout):
    smearingLevels(Nsmear),
    StoutSmearing(Stout),
    SmearedSet(smearingLevels){}

  SmartConf(const Format::Format_G& gf):
    smearingLevels(0),
    StoutSmearing(gf),
    SmearedSet(0){}

  void set_GaugeField(Field& Gauge){
    ThinLinks = &Gauge;
    fill_smearedSet();
  }

  Field smeared_force(const Field&) const;

  Field* get_current_conf() const;

  Field* select_conf(bool smeared) const {
    if (smeared){
      if (smearingLevels)
	return get_current_conf();
      else
	return ThinLinks;
    }
    else
      return ThinLinks;
  }
  

};


#endif
