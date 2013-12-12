/*
 * common_tools.h
 *
 *   Created on: Oct 25, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmar - MCI
 */

#ifndef COMMON_TOOLS_H_
#define COMMON_TOOLS_H_

#ifndef VERBOSITY
#warning "VERBOSITY not defined: Using VERBOSITY=1"
#define VERBOSITY 1
#endif

#define debug(level, str, ...) { if( level <= VERBOSITY ) printf(str, ## __VA_ARGS__); }

#endif // COMMON_TOOLS_H_
