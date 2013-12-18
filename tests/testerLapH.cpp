//------------------------------------------------------------------------
/*!
 * @file testerLapH.cpp 
 * @brief Main source code for testing the LapH classes
 *
 * @author <a href="http://suchix.kek.jp/guido_cossu/">Guido Cossu</a>
 Time-stamp: <2013-12-18 15:34:20 cossu>
 */
//------------------------------------------------------------------------
#include "tests.hpp"
#include "test_LapH.hpp"

int main(int argc, char* argv[]){

  CREATE_TEST(Test_LapH_Solver);
  RUN_TEST;
  CLEAR_TEST; 

  //CREATE_RUN_TEST(Test_LapH_Solver);
  //CLEAR_TEST;

  return 0;
}

