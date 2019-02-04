/*
   AFLSmart - chunks handler
   -------------------------

   Copyright 2018 National University of Singapore

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This implements loading of "chunks" information of a formatted
   file. A chunk is an identifiable section of a formatted file
   amenable to crossover mutation.

 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smart-chunks.h"
#include "smart-utils.h"

#define READ_LINE_BUFFER_SIZE 1024

enum {
  /* 00 */ READ_LINE_OK,
  /* 01 */ READ_LINE_FILE_ERROR,
  /* 02 */ READ_LINE_TOO_LONG
};

/*
 * Returns a hash code for this string, similar to Java's
 * java.lang.String.hashCode().  See:
 * http://grepcode.com/file/repository.grepcode.com/java/root/jdk/openjdk/8u40-b25/java/lang/String.java?av=f#1453
 */
int hash_code(char *str) {
  int h = 0;
  char c;
  
  while ((c = *(str++)) != '\0')
    h = 31 * h + c;
  return h;
}

int next_lower_chunk(char *path, int *length, int *hash, int *type_hash) {
  char c;
  char *s = path;
  char *tmp;
  char *last_underscore;

  if (path == NULL) {
    *length = 0;
    return 0;
  }
  
  c = *s;
  while (c != '~' && c != '\n' && c != '\0' && c != ',') {
    c = *++s;
  }

  *length = s - path;

  if ((tmp = (char *) malloc(*length + 1)) == NULL) {
    *length = 0;
    return 0;
  }
    
  strncpy(tmp, path, *length);
  tmp[*length] = '\0';
  *hash = hash_code(tmp);

  last_underscore = tmp + *length - 1;
  while (last_underscore >= tmp) {
    if (*last_underscore == '_') {
      *last_underscore = '\0';
      break;
    } else if (!isdigit(*last_underscore)) {
      break;
    }
    last_underscore--;
  }

  *type_hash = hash_code(tmp);
  free(tmp);
  
  if (c == '~')
    return -1;

  return 0;  
}

int split_line_on_comma(char *line, int *start_byte, int *end_byte, char **path, char *modifiable) {
  char *start, *end = line;
  char *str = (char *) malloc(READ_LINE_BUFFER_SIZE);

  if (str == NULL)
    return -1;
  
  start = end;
  while (isdigit(*end++));
  strncpy(str, start, end - start - 1);
  str[end - start - 1] = '\0';
  *start_byte = atoi(str);

  start = end;
  while (isdigit(*end++));
  strncpy(str, start, end - start - 1);
  str[end - start - 1] = '\0';
  *end_byte = atoi(str);

  start = end;
  char c;
  do {
    c = *end;
    end++;
  } while (c != '\n' && c != '\0' && c != ',');
  *(end - 1) = '\0';
  *path = start;

  *modifiable = 0;
  if (c == ',') {
    start = end;
    if (!strncmp(start, "Enabled", 7))
      *modifiable = 1;
  }

  free(str);

  return 0;
}

void add_path(struct chunk **tree, char *line) {
  char *next;
  struct chunk *current_chunk = *tree;
  int non_last = -1;
  
  int start_byte, end_byte;
  char modifiable;

  if (split_line_on_comma(line, &start_byte, &end_byte, &next, &modifiable))
    return;

  if (*tree == NULL) {
    int length;
    int h;
    int t;

    non_last = next_lower_chunk(next, &length, &h, &t);

    if (length == 0)
      return;

    next = next + length + 1;

    if ((current_chunk = (struct chunk *) malloc(sizeof(struct chunk))) == NULL)
      return;

    current_chunk->id = h;
    current_chunk->type = t;
    current_chunk->start_byte = -1; /* Unknown */
    current_chunk->end_byte = -1;   /* Unknown */
    current_chunk->modifiable = modifiable;
    current_chunk->next = NULL;
    current_chunk->children = NULL;
  
    *tree = current_chunk;
  } else {
    int length;
    int h;
    int t;

    non_last = next_lower_chunk(next, &length, &h, &t);

    if (length == 0)
      return;

    next = next + length + 1;

    if (current_chunk->id != h) {
      struct chunk *new = (struct chunk *) malloc(sizeof(struct chunk));

      if (new == NULL)
	return;
      
      new->next = current_chunk->next;
      current_chunk->next = new;

      current_chunk = new;
      current_chunk->id = h;
      current_chunk->type = t;
      current_chunk->start_byte = -1; /* Unknown */
      current_chunk->end_byte = -1;   /* Unknown */
      current_chunk->modifiable = modifiable;
      current_chunk->children = NULL;      
    }

    if (!current_chunk->modifiable)
      current_chunk->modifiable = modifiable;
  }

  while (non_last) {
    int length;
    int h;
    int t;

    non_last = next_lower_chunk(next, &length, &h, &t);

    if (length == 0)
      return;

    next = next + length + 1;

    struct chunk *c = current_chunk->children;

    if (c == NULL) {
      if ((c = (struct chunk *) malloc(sizeof(struct chunk))) == NULL) 
	return;
      
      current_chunk->children = c;
      current_chunk = c;
      current_chunk->id = h;
      current_chunk->type = t;
      current_chunk->start_byte = -1; /* Unknown */
      current_chunk->end_byte = -1;   /* Unknown */
      current_chunk->modifiable = modifiable;
      current_chunk->next = NULL;
      current_chunk->children = NULL;
    } else {
      int chunk_found = 0;
      
      do {
        if (c->id == h) {
          current_chunk = c;
          chunk_found = 1;
          break;
        }
        c = c->next;
      } while (c);

      if (!chunk_found) {
	if ((c = (struct chunk *) malloc(sizeof(struct chunk))) == NULL) 
	  return;

	c->next = current_chunk->children;
	current_chunk->children = c;
	current_chunk = c;
        current_chunk->id = h;
	current_chunk->type = t;
        current_chunk->start_byte = -1; /* Unknown */
        current_chunk->end_byte = -1;   /* Unknown */
        current_chunk->modifiable = modifiable;
        current_chunk->children = NULL;
      }
    }

    if (!current_chunk->modifiable)
      current_chunk->modifiable = modifiable;
  }

  current_chunk->start_byte = start_byte;
  current_chunk->end_byte = end_byte;
}

void get_chunks(char *filespec, struct chunk **data_chunks) {
  FILE *chunk_file;
  char *line;
  size_t n;
  
  *data_chunks = NULL;

  if ((chunk_file = fopen(filespec, "r")) == NULL)
    return;

  do {
    line = NULL;
    n = 0;
    if (getline(&line, &n, chunk_file) == -1) {
      free(line);
      line = NULL;
    } else {
      add_path(data_chunks, line);
      if (line != NULL) {
        free(line);
      }
    }
  } while (line != NULL);

  fclose(chunk_file);
}

void delete_chunks(struct chunk *node) {
  struct chunk *sibling = node;

  while (sibling) {
    struct chunk *tmp = sibling->next;

    delete_chunks(sibling->children);
    free(sibling);

    sibling = tmp;
  }
}

struct chunk *copy_chunks(struct chunk *node) {
  if (node == NULL)
    return NULL;

  struct chunk *new_node = (struct chunk *)malloc(sizeof(struct chunk));
  new_node = memcpy(new_node, node, sizeof(struct chunk));
  new_node->next = copy_chunks(node->next);
  new_node->children = copy_chunks(node->children);

  return new_node;
}

void reduce_byte_positions(struct chunk *c, int start_byte, unsigned size) {
  struct chunk *sibling = c;

  while (sibling) {
    if (sibling->start_byte >= 0 && sibling->start_byte > start_byte)
      (sibling->start_byte) -= size;

    if (sibling->end_byte >= 0 && sibling->end_byte >= start_byte)
      (sibling->end_byte) -= size;

    reduce_byte_positions(sibling->children, start_byte, size);

    sibling = sibling->next;
  }
}

void increase_byte_positions_except_target_children(struct chunk *c,
                                                    struct chunk *target,
                                                    int start_byte,
                                                    unsigned size) {
  struct chunk *sibling = c;

  while (sibling) {

    if (sibling->start_byte >= 0 && sibling->start_byte > start_byte)
      (sibling->start_byte) += size;

    if (sibling->end_byte >= 0 && sibling->end_byte >= start_byte)
      (sibling->end_byte) += size;

    if (sibling != target) {
      increase_byte_positions_except_target_children(sibling->children, target,
                                                     start_byte, size);
    }

    sibling = sibling->next;
  }
}

struct chunk *search_and_destroy_chunk(struct chunk *c,
                                       struct chunk *target_chunk,
                                       int start_byte, unsigned size) {
  struct chunk *sibling = c;
  struct chunk *ret = c;
  struct chunk *prev = NULL;

  while (sibling) {

    if (sibling == target_chunk) {
      struct chunk *tmp = sibling->next;

      if (ret == sibling)
        ret = tmp;
      else
        prev->next = tmp;

      delete_chunks(sibling->children);
      free(sibling);

      reduce_byte_positions(tmp, start_byte, size);
      sibling = tmp;
      continue;
    }

    if (sibling->start_byte >= 0 && sibling->start_byte > start_byte)
      (sibling->start_byte) -= size;

    if (sibling->end_byte >= 0 && sibling->end_byte >= start_byte)
      (sibling->end_byte) -= size;

    sibling->children = search_and_destroy_chunk(
        sibling->children, target_chunk, start_byte, size);

    prev = sibling;
    sibling = sibling->next;
  }

  return ret;
}

void print_whitespace(int smart_log_mode, int amount) {
  int i;
  for (i = 0; i < amount; ++i) {
    if (smart_log_mode) {
      smart_log(" ");
    } else {
      printf(" ");
    }
  }
}

void print_node(int smart_log_mode, int hex_mode, struct chunk *node,
                const char *data, int whitespace_amount) {
  while (node != NULL) {
    print_whitespace(smart_log_mode, whitespace_amount);
    if (smart_log_mode) {
      smart_log("Type: %d Start: %d End: %d Modifiable: %d\n", node->type,
                node->start_byte, node->end_byte, node->modifiable);
    } else {
      printf("Type: %d Start: %d End: %d Modifiable: %d\n", node->type,
             node->start_byte, node->end_byte, node->modifiable);
    }
    int chunk_size = node->end_byte - node->start_byte + 1;
    if (data != NULL && node->start_byte >= 0 && node->end_byte >= 0 &&
        chunk_size > 0) {
      if (smart_log_mode) {
        if (hex_mode) {
          smart_log("Data:\n");
          smart_log_n_hex(chunk_size, data + node->start_byte);
        } else {
          smart_log_n(chunk_size, "Data: %s\n", data + node->start_byte);
        }
      } else {
        char *print_data = (char *)malloc(chunk_size + 1);
        strncpy(print_data, data + node->start_byte, chunk_size);
        print_data[chunk_size] = '\0';
        print_whitespace(smart_log_mode, whitespace_amount);
        printf("Data: %s\n", print_data);
        free(print_data);
      }
    }
    if (node->children) {
      print_node(smart_log_mode, hex_mode, node->children, data,
                 whitespace_amount + 4);
    }
    node = node->next;
  }
}

void print_tree(struct chunk *root) { print_tree_with_data(root, NULL); }

void print_tree_with_data(struct chunk *root, const char *data) {
  print_node(0, 0, root, data, 0);
}

void smart_log_tree(struct chunk *root) {
  smart_log_tree_with_data(root, NULL);
}

void smart_log_tree_with_data(struct chunk *root, const char *data) {
  print_node(1, 0, root, data, 0);
}

void smart_log_tree_hex(struct chunk *root) {
  smart_log_tree_with_data_hex(root, NULL);
}

void smart_log_tree_with_data_hex(struct chunk *root, const char *data) {
  print_node(1, 1, root, data, 0);
}
