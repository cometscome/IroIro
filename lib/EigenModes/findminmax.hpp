/*! 
  @file findminmax.hpp
  @brief Find the minimum and maximum eigenmode of a real matrix
*/
#include <memory>
#include "Fopr/fopr.h"
#include "Tools/randNum.h"

struct MinMaxOut {
  double min;
  double max;
};

class findMinMax {
  Fopr* Kernel_;
  std::auto_ptr<const RandNum> rand_;
  size_t vect_size;

  findMinMax(); //hide default constructor
public:
  /*! @brief Default constructor - Fopr should have real eigenmodes */
  explicit findMinMax(Fopr*, const RandNum*, size_t) ;

  ~findMinMax();
  double findMin(const double&) const;
  double findMax() const;
  
  MinMaxOut findExtrema() const;
};
