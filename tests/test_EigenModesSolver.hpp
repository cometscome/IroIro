/*! @file test_EigenModesSolver.hpp
 *  @brief Declaration of classes for testing the EigenModesSolver classes
 */
#ifndef TEST_EIGENMODESSOLVER_INCLUDED
#define TEST_EIGENMODESSOLVER_INCLUDED

#include "include/common_code.hpp"
#include "tests/tests.hpp"
#include "Communicator/comm_io.hpp"

class Test_EigenModesSolver: public TestGeneral{
 private:
  XML::node node_;
  GaugeField conf_;
  std::string output_;
 public:
  Test_EigenModesSolver(XML::node node,const GaugeField& conf,
			const RandNum&, std::string file)
    :node_(node),conf_(conf),output_(file){
    CCIO::cout<<"Test_EigenModesSolver called"<<std::endl;
  }
  int run();
};

#endif
