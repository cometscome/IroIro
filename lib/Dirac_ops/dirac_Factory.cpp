#include "dirac_Factory.hpp"

#include <string.h>
#include "Communicator/comm_io.hpp"

namespace DiracOperators {
  DiracOperatorFactory* createDiracOperatorFactory(const XML::node node)
  {
    if (node !=NULL) {
      
      const char* Dirac_name = node.attribute("name").value();
      
      if (!strcmp(Dirac_name, "DiracWilson")) { 
	return new DiracWilsonFactory(node);
      }
      if (!strcmp(Dirac_name, "DiracWilson_EvenOdd")) { 
	return new DiracWilsonEvenOddFactory(node);
      }
      if (!strcmp(Dirac_name, "DiracOptimalDomainWall4d")) { 
	return new DiracDWF4dFactory(node);
      }
      if (!strcmp(Dirac_name, "DiracOptimalDomainWall5d")) { 
	return new DiracDWF5dFactory(node);
      }
      
      std::cerr << "No Dirac Operator available with name ["
		<< Dirac_name << "]" << std::endl;
      abort();
      
    } else {
      std::cout << "Mandatory node is missing in input file (Dirac Object)"
		<< std::endl;
      abort();
    }
  }
}
