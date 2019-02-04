/*
   AFLSmart - Utility functions
   ----------------------------

   Copyright 2018 National University of Singapore

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

*/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct log_file {
  pid_t pid;
  FILE *file;
  struct log_file *left;
  struct log_file *right;
};

static struct log_file *log_file_set = NULL;

static char *dirname = NULL;

FILE *get_new_log_file(pid_t pid) {
  char filespec[100];
  FILE *file;

  if (sprintf(filespec, "%s/%u.log", dirname, pid) < 0)
    return NULL;

  if ((file = fopen(filespec, "w")) == NULL)
    return NULL;

  return file;
}

struct log_file *get_node(FILE *file, pid_t pid) {
  struct log_file *node;

  if (file == NULL)
    return NULL;

  if ((node = (struct log_file *)malloc(sizeof(struct log_file))) == NULL)
    return NULL;

  node->pid = pid;
  node->file = file;
  node->left = NULL;
  node->right = NULL;

  return node;
}

FILE *get_log_file_recursively(struct log_file *node, pid_t pid) {
  if (node == NULL)
    return NULL;

  if (pid == node->pid)
    return node->file;

  if (pid < node->pid) {
    if (node->left == NULL) {
      FILE *file = get_new_log_file(pid);
      struct log_file *new_node = get_node(file, pid);

      node->left = new_node;
      return file;
    } else {
      return get_log_file_recursively(node->left, pid);
    }
  } else if (pid > node->pid) {
    if (node->right == NULL) {
      FILE *file = get_new_log_file(pid);
      struct log_file *new_node = get_node(file, pid);

      node->right = new_node;
      return file;
    } else {
      return get_log_file_recursively(node->right, pid);
    }
  }

  return NULL;
}

/*
  Finds a log file to write to, if not found, then add a new one for
  the current process id.
 */
FILE *get_log_file() {
  pid_t pid = getpid();

  /* Initialization is not done */
  if (log_file_set == NULL)
    return NULL;

  return get_log_file_recursively(log_file_set, pid);
}

char *read_file(const char *filespec) {
  FILE *file;
  long file_size;
  char *file_content;

  if ((file = fopen(filespec, "r")) == NULL) {
    /* Error opening file for reading. */
    return NULL;
  }

  fseek(file, 0L, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0L, SEEK_SET);

  file_content = (char *)malloc(file_size + 1);
  if (file_content == NULL) {
    fclose(file);
    /* Memory allocation error */
    return NULL;
  }

  if (fread(file_content, 1, file_size, file) != file_size) {
    free(file_content);
    fclose(file);
    /* Error reading file content */
    return NULL;
  }

  file_content[file_size] = '\0';
  fclose(file);

  return file_content;
}

void print_segment(const char *s, int start_byte, int end_byte) {
  int current = 1;

  while (*s) {
    if (current >= start_byte && current <= end_byte) {
      printf("%c", *s);
    }
    s++;
    current++;
  }
}

void smart_log_init(const char *out_dir) {
  pid_t pid = getpid();
  char *tmp_dirname;
  const char *s;
  FILE *file;
  int len;

  /* Already initialized */
  if (log_file_set != NULL)
    return;

  if (out_dir == NULL)
    return;

  s = out_dir;
  len = 0;
  while (*s) {
    if (len == 1000)
      break;
    s++;
    len++;
  }

  /* out_dir is too long */
  if (*s)
    return;

  tmp_dirname = (char *)malloc(len + strlen("/log") + 1);
  sprintf(tmp_dirname, "%s/log", out_dir);

  if (mkdir(tmp_dirname, S_IRWXU) < 0 && errno != EEXIST)
    return;

  dirname = tmp_dirname;

  file = get_new_log_file(pid);
  log_file_set = get_node(file, pid);
}

/* Function to log a string in <out_dir>/log/<process id>.log. */
void smart_log(const char *format, ...) {
  if (format == NULL)
    return;

  /* Initialization is not done */
  if (log_file_set == NULL)
    return;

  va_list args;
  va_start(args, format);

  FILE *log_file = get_log_file();

  if (log_file == NULL)
    return;

  vfprintf(log_file, format, args);
}

/* Function to log a bounded string in <out_dir>/log/<process id>.log. */
void smart_log_n(size_t size, const char *format, ...) {
  if (size <= 0 || format == NULL)
    return;

  /* Initialization is not done */
  if (log_file_set == NULL)
    return;

  va_list args;
  va_start(args, format);

  FILE *log_file = get_log_file();

  if (log_file == NULL)
    return;

  char *buf = (char *)malloc(size + 1);
  vsnprintf(buf, size, format, args);
  fprintf(log_file, "%s", buf);
  free(buf);
}

void smart_log_hex(const char *s) {
  if (s == NULL)
    return;

  /* Initialization is not done */
  if (log_file_set == NULL)
    return;

  FILE *log_file = get_log_file();

  if (log_file == NULL)
    return;

  unsigned i = 0;
  while (*s) {
    if (i % 16 == 0) {
      if (i > 0) {
        fprintf(log_file, "\n");
      }
      fprintf(log_file, "%p: ", (void *)s);
    }
    fprintf(log_file, "%02x ", ((*s) & 0xff));
    s++;
    i++;
  }

  fprintf(log_file, "\n");
  fflush(log_file);
}

void smart_log_n_hex(size_t size, const char *s) {
  unsigned i;

  if (s == NULL || size == 0)
    return;

  /* Initialization is not done */
  if (log_file_set == NULL)
    return;

  FILE *log_file = get_log_file();

  if (log_file == NULL)
    return;

  for (i = 0; i < size; ++i) {
    if (i % 16 == 0) {
      if (i > 0) {
        fprintf(log_file, "\n");
      }
      fprintf(log_file, "%p: ", (void *)s);
    }
    fprintf(log_file, "%02x ", ((*s) & 0xff));
    s++;
  }

  fprintf(log_file, "\n");
  fflush(log_file);
}

void smart_log_int_hex(long number) {
  char *s = (char *)&number;
  unsigned i;

  FILE *log_file = get_log_file();

  if (log_file == NULL)
    return;

  for (i = 0; i < 4; ++i) {
    fprintf(log_file, "%02x ", ((*s) & 0xff));
    s++;
  }
  fprintf(log_file, "\n");
  fflush(log_file);
}
