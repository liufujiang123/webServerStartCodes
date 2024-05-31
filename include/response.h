#ifndef __RESPONSE_H__
#define __RESPONSE_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/mman.h>
#include "buffer.h"
#include "parse.h"
// 对请求的信息进行拆解

typedef enum Return_value
{
  CORRECT,
  ERROR,
  CLOSE,
  CLOSE_FROM_CLIENT,
  PERSISTENT,
  NOT_COMPLETE
} Return_value;

typedef enum METHODS
{
  GET,
  HEAD,
  POST,
  NOT_SUPPORTED
} METHODS;
typedef enum Connection_status
{
  KeepAlive,
  Close
} Connection_status;

int handle_request(int, int, dynamic_buffer *, struct sockaddr_in);
void helper_head(Request *, dynamic_buffer *, struct sockaddr_in);

int handle_get(Request *, dynamic_buffer *, struct sockaddr_in, int);
int handle_head(Request *, dynamic_buffer *, struct sockaddr_in, int);
int handle_post(Request *, dynamic_buffer *, struct sockaddr_in, int);

void handle_400(dynamic_buffer *, struct sockaddr_in);
void handle_404(dynamic_buffer *, struct sockaddr_in);
void handle_501(dynamic_buffer *, struct sockaddr_in);
void handle_505(dynamic_buffer *, struct sockaddr_in);
void set_response(dynamic_buffer *, char *, char *);
void set_header(dynamic_buffer *, char *, char *);
void set_msg(dynamic_buffer *, char *, int);

void free_request(Request *);
int get_file_content(dynamic_buffer *, dynamic_buffer *);
char *get_header_value(Request *, char *);
int check_method(char *method);
#endif