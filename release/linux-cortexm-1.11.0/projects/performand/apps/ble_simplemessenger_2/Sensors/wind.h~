/*
 * wind.h
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */


static long Wind_serviceHdls[] = {0x0040, 0x0044, 0x0048, 0x0037};
static char* Wind_serviceHdlCharDesc[] = {"Battery", "Speed", "Direction", "Temperature"};
static int Wind_nServiceHdls = 4;

int Wind_initialize(void);
int Wind_parseData(char* data, int *i, char* mm_str);
