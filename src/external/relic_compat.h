#pragma once

/* ---------------------------------------------------------
 * Save and undefine PIVX macros BEFORE including relic
 * --------------------------------------------------------- */

#ifdef VERSION
#define _PIVX_SAVED_VERSION VERSION
#undef VERSION
#endif

#ifdef PACKAGE_NAME
#define _PIVX_SAVED_PACKAGE_NAME PACKAGE_NAME
#undef PACKAGE_NAME
#endif

#ifdef PACKAGE_VERSION
#define _PIVX_SAVED_PACKAGE_VERSION PACKAGE_VERSION
#undef PACKAGE_VERSION
#endif

#ifdef PACKAGE_STRING
#define _PIVX_SAVED_PACKAGE_STRING PACKAGE_STRING
#undef PACKAGE_STRING
#endif

#ifdef PACKAGE_TARNAME
#define _PIVX_SAVED_PACKAGE_TARNAME PACKAGE_TARNAME
#undef PACKAGE_TARNAME
#endif

#ifdef PACKAGE_URL
#define _PIVX_SAVED_PACKAGE_URL PACKAGE_URL
#undef PACKAGE_URL
#endif

#ifdef PACKAGE_BUGREPORT
#define _PIVX_SAVED_PACKAGE_BUGREPORT PACKAGE_BUGREPORT
#undef PACKAGE_BUGREPORT
#endif

/* ---------------------------------------------------------
 * Now include relic (it will define its own versions)
 * --------------------------------------------------------- */
#include <relic_conf.h>

/* ---------------------------------------------------------
 * Undefine RELIC macros
 * --------------------------------------------------------- */
#undef VERSION
#undef PACKAGE_NAME
#undef PACKAGE_VERSION
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_BUGREPORT

/* ---------------------------------------------------------
 * Restore PIVX macros if they were defined
 * --------------------------------------------------------- */
#ifdef _PIVX_SAVED_VERSION
#define VERSION _PIVX_SAVED_VERSION
#undef _PIVX_SAVED_VERSION
#endif

#ifdef _PIVX_SAVED_PACKAGE_NAME
#define PACKAGE_NAME _PIVX_SAVED_PACKAGE_NAME
#undef _PIVX_SAVED_PACKAGE_NAME
#endif

#ifdef _PIVX_SAVED_PACKAGE_VERSION
#define PACKAGE_VERSION _PIVX_SAVED_PACKAGE_VERSION
#undef _PIVX_SAVED_PACKAGE_VERSION
#endif

#ifdef _PIVX_SAVED_PACKAGE_STRING
#define PACKAGE_STRING _PIVX_SAVED_PACKAGE_STRING
#undef _PIVX_SAVED_PACKAGE_STRING
#endif

#ifdef _PIVX_SAVED_PACKAGE_TARNAME
#define PACKAGE_TARNAME _PIVX_SAVED_PACKAGE_TARNAME
#undef _PIVX_SAVED_PACKAGE_TARNAME
#endif

#ifdef _PIVX_SAVED_PACKAGE_URL
#define PACKAGE_URL _PIVX_SAVED_PACKAGE_URL
#undef _PIVX_SAVED_PACKAGE_URL
#endif

#ifdef _PIVX_SAVED_PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT _PIVX_SAVED_PACKAGE_BUGREPORT
#undef _PIVX_SAVED_PACKAGE_BUGREPORT
#endif