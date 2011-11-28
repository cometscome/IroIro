/*!
 * @file fermion_meas_factory.cpp 
 *
 * @brief Fermionic measurements operators factories selector
 */

#include "fermion_meas_factory.hpp"

#include <string.h>

namespace QuarkPropagators {
  QuarkPropagatorCreator* createQuarkPropagatorFactory(XML::node node)
  {
    
    if (node !=NULL) {
      
      const char* QuarkProp_name = node.attribute("name").value();
      
      if (!strcmp(QuarkProp_name, "Qprop")) { 
        return new QPropCreator(node);
      }
      if (!strcmp(QuarkProp_name, "Qprop_EvenOdd")) { 
        return new QPropCreator_EvenOdd(node);
      }
      if (!strcmp(QuarkProp_name, "QpropDWF")) { 
        return new QPropDWFCreator(node);
      }

    std::cerr << "No Quark Propagator available with name ["
              << QuarkProp_name << "]" << std::endl;
    abort();
    
    } else {
      std::cout << "Requested node is missing in input file "
		 << "(QuarkPropagator Object)\n" 
		 << "Request by " << node.parent().name() << std::endl;
      abort();
    }
  }  
};
