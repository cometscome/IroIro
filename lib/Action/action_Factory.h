#ifndef ACTION_FACT_
#define ACTION_FACT_

#include "action.h"
#include "include/format_G.h"

/*!
 *@class ActionFactory
 *
 *@brief Abstract Factory class for General actions
 *
 */
class ActionFactory {
public:
  /*!
    Virtual function returning the %Action
  */
  virtual Action* getAction(const Format::Format_G& GaugeFormat,
			    Field* const GaugeField) = 0;
};


#endif
