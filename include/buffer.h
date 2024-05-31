// 动态buffer
// 初始化
// 重置
//
// 追加
// 释放
//

#ifndef BUFFER_H
#define BUFFER_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DEFAULT_CAPACITY 1024

typedef struct dynamic_buffer
{
  char *buf;
  size_t capacity;
  size_t current_size;
} dynamic_buffer;

void init_dynamic_buffer(dynamic_buffer *);
void memset_dynamic_buffer(dynamic_buffer *);
void free_dynamic_buffer(dynamic_buffer *);
void append_dynamic_buffer(dynamic_buffer *, char *, size_t);
void print_dynamic_buffer(dynamic_buffer *);
void reset_dynamic_buffer(dynamic_buffer *);
void update_dynamic_buffer(dynamic_buffer *, char *);

#endif