/******************************************************************************
 * echo_server.c                                                               *
 *                                                                             *
 * Description: This file contains the C source code for an echo server.  The  *
 *              server runs on a hard-coded port and simply write back anything*
 *              sent to it by connected clients.  It does not support          *
 *              concurrent clients.                                            *
 *                                                                             *
 * Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
 *          Wolf Richter <wolf@cs.cmu.edu>                                     *
 *                                                                             *
 *******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "response.h"

#define ECHO_PORT 9999
#define BUF_SIZE 4096

char *dest = "\r\n\r\n";
int close_socket(int sock)
{
  if (close(sock))
  {
    fprintf(stderr, "Failed closing socket.\n");
    return 1;
  }
  return 0;
}
int extract_http_messages(char **request_begin, dynamic_buffer *dbuf, int client_sock)
{
  char *t = dbuf->buf, *temp = dbuf->buf;
  int body_start = 0;
  if ((t = strstr(temp, dest)) == NULL)
  {
    return 0;
  }
  t += strlen(dest);
  body_start = t - temp;
  Request *request = parse(dbuf->buf, t - temp, client_sock);
  if (request == NULL)
  {
    return 1;
  }
  int body_length = 0;
  // 假设存在Content-Length
  if (get_header_value(request, "Content-Length") != NULL)
  {
    body_length = atoi(get_header_value(request, "Content-Length"));
  }
  if (body_length + body_start > dbuf->current_size)
  {
    return 0;
  }
  *request_begin = dbuf->buf + body_length + body_start;
  return 1;
}
int deal_buf(dynamic_buffer *dbuf, size_t readret, int client_sock, int sock, int fd_in, struct sockaddr_in cli_addr)
{
  // 这个函数直接返回意味着发送失败

  //  读取并处理完整的请求
  //  发送响应报文
  //  将不完整的尾巴写会dbuf
  //  返回请求执行后是否断开连接
  //  1.请求断开连接 2.服务器问题导致的断开连接

  // char *t = dbuf->buf, *temp = dbuf->buf;

  // while直接退出意味着这个请求不完整

  // debug
  // printf("into the extract messages\n");
  // 取出每一个完整请求(包含请求体)
  dynamic_buffer *each = (dynamic_buffer *)malloc(sizeof(dynamic_buffer));
  init_dynamic_buffer(each);

  // size_t len = t - temp;
  append_dynamic_buffer(each, dbuf->buf, readret);
  // append_dynamic_buffer(each, dest, strlen(dest));

  // temp = t + strlen(dest);
  //  从请求中得到下一步的状态，将即将发送的响应报文写入each
  int return_value = handle_request(client_sock, sock, each, cli_addr);

  // 每处理一个请求就发送,即使是关闭连接请求依旧要回复响应报文
  if (send(client_sock, each->buf, each->current_size, 0) != each->current_size)
  {
    // Something is wrong
    close_socket(client_sock);
    close_socket(sock);
    free_dynamic_buffer(each);
    fprintf(stderr, "ERROR Sending Back to Client\n");

    return ERROR;
  }
  // 如果是请求报文的问题或者请求报文请求关闭连接
  if (return_value == CLOSE || return_value == CLOSE_FROM_CLIENT)
  {
    free_dynamic_buffer(each);
    // 打破循环，不再处理后续当前dbuf中的请求
    // debug
    printf("return_value is close\n");
    return CLOSE;
  }
  // update_dynamic_buffer(dbuf, temp);
  // while退出循环意味着1.请求不完全 2.请求没有了

  return PERSISTENT;
}

int main(int argc, char *argv[])
{
  int sock, client_sock;
  ssize_t readret;
  socklen_t cli_size;
  struct sockaddr_in addr, cli_addr;
  char buf[BUF_SIZE];

  fprintf(stdout, "----- Echo Server -----\n");
  /* all networked programs must create a socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    fprintf(stderr, "Failed creating socket.\n");
    return EXIT_FAILURE;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(ECHO_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  /* servers bind sockets to ports---notify the OS they accept connections */
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))
  {
    close_socket(sock);
    fprintf(stderr, "Failed binding socket.\n");
    return EXIT_FAILURE;
  }

  if (listen(sock, 5))
  {
    close_socket(sock);
    fprintf(stderr, "Error listening on socket.\n");
    return EXIT_FAILURE;
  }

  /* finally, loop waiting for input and then write it back */
  while (1)
  {

    cli_size = sizeof(cli_addr);
    if ((client_sock = accept(sock, (struct sockaddr *)&cli_addr,
                              &cli_size)) == -1)
    {
      close(sock);
      fprintf(stderr, "Error accepting connection.\n");
      return EXIT_FAILURE;
    }

    readret = 0;
    dynamic_buffer *dbuf = (dynamic_buffer *)malloc(sizeof(dynamic_buffer));
    init_dynamic_buffer(dbuf);

    // 接收处理循环，动态缓冲区是在持续连接情况下为了解决请求过长以及存储多个请求
    // recv函数的终止条件：1.接收缓冲区中的数据已经被读取完毕2.指定的接收缓冲区已经被填满
    while ((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
    {
      // 持续连接缓冲区
      // debug
      printf("before append\n");
      append_dynamic_buffer(dbuf, buf, readret);
      // debug
      printf("it's how many times receive\n");
      // 进行解析和响应
      if (deal_buf(dbuf, readret, client_sock, sock, 8192, cli_addr) != PERSISTENT)
      {
        // 决定是否断开在这个端口的连接
        memset(buf, 0, BUF_SIZE);
        // debug
        printf("decide to break the circle\n");
        break;
      }
      memset(buf, 0, BUF_SIZE);
    }
    printf("the while is broken\n");
    if (readret == -1)
    {
      close_socket(client_sock);
      close_socket(sock);
      fprintf(stderr, "Error reading from client socket.\n");
      return EXIT_FAILURE;
    }

    if (close_socket(client_sock))
    {
      close_socket(sock);
      fprintf(stderr, "Error closing client socket.\n");
      return EXIT_FAILURE;
    }
  }

  close_socket(sock);

  return EXIT_SUCCESS;
}
