/*!
 * @file test_Overlap.hpp
 *
 * @brief Definition of the class to test the overlap functions
 */

#ifndef TEST_OVERLAP_INCLUDED
#define TEST_OVERLAP_INCLUDED

#include "include/iroiro_code.hpp"
#include "tests.hpp"


class Test_Overlap {
 private:
  XML::node OverlapNode;
  GaugeField& Gauge;

  int sign_subt();
  // test of sign function with low-mode subtraction

  int ovsolver_subt();
  // test of overlap solver

 public:
  Test_Overlap(XML::node node,
	       GaugeField& u):OverlapNode(node),
			      Gauge(u){}
  
  int run();
    


};

#endif
