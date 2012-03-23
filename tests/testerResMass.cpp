/*!
 * @file testerQropMom.cpp 
 * @brief Main source code for testing the S(p) calculation
 */
#include "test_ResidualMass.hpp"
#include "include/commandline.hpp"

using namespace XML;
using namespace MapsEnv;

int main(int argc, char* argv[]){

  CommandOptions Options = ReadCmdLine(argc, argv);
  
  //Reading input file
  node top_node = getInputXML(Options.filename);  

  //Initializing geometry using XML input
  Geometry geom(top_node);
  initialize_mapper();

  //Initialize GaugeField using XML input
   GaugeGlobal GaugeF(geom);
   GaugeF.initialize(top_node);

   node Mres_node = top_node;
   descend(Mres_node, "TestResMass");

   Test_ResMass ResMassTest(Mres_node, GaugeF);
   ResMassTest.run();
   
   return 0;
 }
