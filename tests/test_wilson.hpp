/*!
 * @file test_wilson.hpp
 *
 * @brief Tests for the propagators and sources
 *
 */
#ifndef TEST_WILSON_INCLUDED
#define TEST_WILSON_INCLUDED

#include "include/common_code.hpp"
#include "tests.hpp"

class Test_Wilson : public TestGeneral{
private:
  XML::node Wilson_node_;
  GaugeField& conf_;
public:
  Test_Wilson(const XML::node node, GaugeField& conf)
    :Wilson_node_(node),
     conf_(conf){}

  ~Test_Wilson(){}
  
  int run();
};

#endif
