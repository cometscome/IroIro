//------------------------------------------------------------------------
/*!
 * @file testerWilson.cpp 
 * @brief Main source code for testing the Wilson classes
 * @author <a href="http://suchix.kek.jp/guido_cossu/">Guido Cossu</a>
 */
//------------------------------------------------------------------------

#include "test_wilson_Brillouin_Imp.hpp"

using namespace XML;

int main(){
  
  //Reading input file
  node top_node = getInputXML("test_Wilson_Brillouin.xml");  

  //Initializing geometry using XML input
  Geometry geom(top_node);

  //Initialize GaugeField using XML input
  GaugeGlobal GaugeF(geom);
  GaugeF.initialize(top_node);

  /////////////
  
  node Wilson_node = top_node;
  descend(Wilson_node, "TestWilsonBrillouin_Imp");
    
  Test_Wilson_Brillouin_Imp WilsonTest(Wilson_node, GaugeF);
  WilsonTest.run();

  return 0;
}

