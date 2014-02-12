/*
 * verbosity.h
 *
 *   Created on: Oct 25, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmar - MCI
 */

#ifndef VERBOSITY_H_
#define VERBOSITY_H_

#ifndef VERBOSITY
#warning "VERBOSITY not defined: Using VERBOSITY=1"
#define VERBOSITY 1
#endif

#define debug(level, str, ...) { if( level <= VERBOSITY ) printf(str, ## __VA_ARGS__); }

#define printe(str, ...) { fprintf(stderr, "[%s] ERROR: " str, programName, ## __VA_ARGS__); }
#define printw(str, ...) { fprintf(stderr, "[%s] WARNING: " str, programName, ## __VA_ARGS__); }

#endif // VERBOSITY_H_
