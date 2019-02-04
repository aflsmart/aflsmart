/*
   AFLSmart - Utility functions
   ----------------------------

   Copyright 2018 National University of Singapore

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

*/

#ifndef __SMART_UTILS_H
#define __SMART_UTILS_H 1

/* Please do not forget to free the returned string.
 */
extern char *read_file(const char *filespec);

/* Print the segment of a string on the screen using printf() */
extern void print_segment(const char *s, int start_byte, int end_byte);

/* Logging initialization. */
extern void smart_log_init(const char *out_dir);

/* Function to log a string in <out_dir>/log/<process id>.log. */
extern void smart_log(const char *format, ...);

/* Function to log a bounded string in <out_dir>/log/<process id>.log. */
extern void smart_log_n(size_t size, const char *format, ...);

/* Function to log in hex a string in <out_dir>/log/<process id>.log. */
extern void smart_log_hex(const char *s);

/* Function to log in hex a bounded string in <out_dir>/log/<process id>.log. */
extern void smart_log_n_hex(size_t size, const char *s);

/* Function to log in hex a number in <out_dir>/log/<process id>.log. */
extern void smart_log_num_hex(long n);

#endif
