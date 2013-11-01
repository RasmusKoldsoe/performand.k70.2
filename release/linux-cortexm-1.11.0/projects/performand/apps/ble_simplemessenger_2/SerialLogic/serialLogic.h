/*
 * serialLogic.h
 *
 *   Created on: Sep 19, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark
 */

#ifndef SERIALLOGIC_H_
#define SERIALLOGIC_H_

extern int open_serial(char *port, int oflags);
extern void close_serial(int fd);
extern void *read_serial(void *_bleCentral); // thread
extern void *write_serial(void *_bleCentral); // thread

#endif /* SERIALLOGIC_H_ */
