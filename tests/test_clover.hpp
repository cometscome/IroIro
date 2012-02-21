/*!
 * @file test_clover.hpp
 * @brief Tests for the propagators and sources
 */
#ifndef TEST_CLOVER_INCLUDED
#define TEST_CLOVER_INCLUDED

#include "include/common_code.hpp"
#include "tests.hpp"

class Test_Clover : public TestGeneral{
private:
  XML::node clover_node_;
  GaugeField& conf_;
public:
  Test_Clover(const XML::node node, GaugeField& conf)
    :clover_node_(node),
     conf_(conf){}

  ~Test_Clover(){}
  
  int run();
};

#endif
