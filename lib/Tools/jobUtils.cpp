/*! @file jobUtils.cpp
 * @brief implementation of member functions of JobUtils
 */
#include "jobUtils.hpp"
#include "lib/Communicator/comm_io.hpp"
#include "lib/Communicator/communicator.hpp"
#include <fstream>

namespace JobUtils{
  void echo_input(const char* file_name){
    
    if (Communicator::instance()->primaryNode()){
    
    std::ifstream input(file_name);
    std::string buf;
    std::cout<<"==== [begin] Contents of the input file "
	      << file_name << "====\n";

    while(input && getline(input,buf))
      std::cout<< buf << "\n";

    std::cout<<"==== [end] Contents of the input file "
	      << file_name << "====\n"
	      << std::fflush;
    }

    Communicator::instance()->sync();
    return;
  }
}
