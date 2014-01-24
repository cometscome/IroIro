/*!
 * @file utils_DWF4d.hpp
 * @brief declaration of utility functions for 4D-DWF op.
 */
class Dirac_DomainWall_4D;
class Field;

namespace Utils_DWF4D{
  const Field delta(const Dirac_DomainWall_4D&, const Field&);
  const Field signKernel(const Dirac_DomainWall_4D&, const Field&);
}
