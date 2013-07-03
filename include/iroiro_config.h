#ifndef IROIRO_CONFIG_H
#define IROIRO_CONFIG_H

/* Undef the unwanted from the environment -- eg the compiler command line */

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

/* Include the stuff generated by autoconf - internal */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Prefix everything with IROIRO_ */
static const char* const IROIRO_PACKAGE =  PACKAGE;
static const char* const IROIRO_PACKAGE_BUGREPORT = PACKAGE_BUGREPORT;
static const char* const IROIRO_PACKAGE_NAME = PACKAGE_NAME;
static const char* const IROIRO_PACKAGE_STRING = PACKAGE_STRING;
static const char* const IROIRO_PACKAGE_TARNAME = PACKAGE_TARNAME;
static const char* const IROIRO_PACKAGE_VERSION = PACKAGE_VERSION;


/* Undef the unwanted */
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION





#endif  /* Close starting ifnded */
