/*! @file test_MesonSpectrum.hpp
    @brief Declaration of test code for meson spectrum
*/
#ifndef TEST_MESONSPECTRUM_INCLUDED
#define TEST_MESONSPECTRUM_INCLUDED

#include "include/iroiro_code.hpp"
class RandNum;

class Test_MesonSpectrum {
private:
  XML::node node_;
  GaugeField conf_;
  std::string output_;
public:
  Test_MesonSpectrum(XML::node node,const GaugeField& conf,
		     const RandNum&,std::string file)
    :node_(node),conf_(conf),output_(file){}
  int run();
};

#endif


