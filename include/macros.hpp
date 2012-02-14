/*!
 * @file macros.hpp
 *
 * @brief Include several useful macros and definitions
 *
 */

#ifndef MACROS_HPP_
#define MACROS_HPP_

#include <sys/time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <sstream>


// Predefined verbosity levels
#define ACTION_VERB_LEVEL 1
#define SOLV_ITER_VERB_LEVEL 2
#define SOLV_MONITOR_VERB_LEVEL 1
#define DEBUG_VERB_LEVEL 5

#define NC_ 3  //Number of colours, now fixed here
#define NDIM_ 4  //Number of lattice dimensions, now fixed here

#define TIMING_START				\
  timeval start, end;				\
  gettimeofday(&start,NULL)

#define TIMING_END(var)							\
  gettimeofday(&end,NULL);						\
  var = (end.tv_sec - start.tv_sec) * 1000.0;				\
  var = var + (end.tv_usec - start.tv_usec) / 1000.0   // us to ms

class NullType{};

#endif
