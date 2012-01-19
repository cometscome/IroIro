//------------------------------------------------------------------------
/*!
 * @file testerEigenModes.cpp 
 * @brief Main source code for testing the EigenModes classes
 *
 * @author <a href="http://suchix.kek.jp/guido_cossu/">Guido Cossu</a>
 */
//------------------------------------------------------------------------

#include "test_EigenModes_IRL.hpp"
#include "include/commandline.hpp"

using namespace XML;

int main(int argc, char* argv[]){

  CommandOptions Options = ReadCmdLine(argc, argv);
  
  //Reading input file
  node top_node = getInputXML(Options.filename);  

  //Initializing geometry using XML input
  Geometry geom(top_node);

  //Initialize GaugeField using XML input
  GaugeField GaugeF(geom);
  GaugeF.initialize(top_node);

  /////////////
  
  node Eigen_node = top_node;
  descend(Eigen_node, "EigenTest");
  
  
  Test_EigenModes_IRL EigenTest(GaugeF);
  EigenTest.run(Eigen_node);

  return 0;
}

