#ifndef APRSIM_H
#define APRSIM_H

#define PROGRAM_NAME "Apricos Simulator"

#ifndef VERSION
#define VERSION "_UNKNOWN_"
#endif

#define PROGRAM_VERSION_STRING PROGRAM_NAME " V" VERSION

#define MOUSE_SCROLL_LINES 3

/* RUN mode executes instructions in slices of this much wall time between
 * keyboard polls and screen updates (~60 updates per second) */
#define RUN_SLICE_USEC 16667
#define RUN_SLICE_CHECK_INTERVAL 1024

#endif /* APRSIM_H */
