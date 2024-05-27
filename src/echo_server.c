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
#include "parse.h"

#define ECHO_PORT 9999
#define BUF_SIZE 4096

const char *_400msg = "HTTP/1.1 400 Bad request\r\n\r\n";
const char *_404msg = "HTTP/1.1 404 Not Found\r\n\r\n";
const char *_501msg = "HTTP/1.1 501 Not Implemented\r\n\r\n";
const char *_505msg = "HTTP/1.1 505 HTTP Version not supported\r\n\r\n";
const char *ok = "HTTP/1.1 200 OK\r\n";

int check_method(char *method)
{
  if (!strcmp(method, "GET"))
    return 1;
  if (!strcmp(method, "HEAD"))
    return 2;
  if (!strcmp(method, "POST"))
    return 3;
  return 0;
}
// 读取文件并判断是否成功
bool read_check(char *src, char *dest)
{

  FILE *file = fopen(src, 'r');
  // 从文件末尾偏移，偏移0位
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file); // ftell()存储当前文件描述符的读取的偏移位置，这里就是文件末尾
  int readRet1 = fread(dest, sizeof(char), file_size, file);
  if (readRet1 < 0)
  {
    // read函数失败，可以检查errno来确定失败的原因
    switch (errno)
    {
    case EACCES:
      printf("Permission denied\n");
      break;
    case ENOENT:
      printf("File does not exist\n");
      break;
    case EIO:
      printf("An I/O error occurred\n");
      break;
    default:
      printf("An unknown error occurred\n");
      break;
    }
    return false;
  }
  return true;
}
char *handle_request(Request *request, char *buf)
{
  if (request == NULL)
  {
    // 格式错误
    strcpy(buf, _400msg);
  }
  else if (strcmp(request->http_version, "HTTP/1.1"))
  {
    // printf("HTTP%s", request->http_version);
    strcpy(buf, _505msg);
  }
  else if (strcmp(request->http_uri, "/"))
  {
    strcpy(buf, _404msg);
  }
  else if (check_method(request->http_method) == 1)
  {
    // get
    strcpy(buf, ok);
    // 读取url指定的文件
    read_check(request->http_uri, buf + strlen(ok));
    // TODO 对读取长度进行处理，避免溢出
  }
  else if (check_method(request->http_method) == 2)
  {
    // head
    strcpy(buf, ok);
  }
  else if (check_method(request->http_method) == 3)
  {
    // post
    strcpy(buf, ok);
    read_check(request->http_uri, buf + strlen(ok));
  }
  else if (!check_method(request->http_method))
  {
    // 不支持的请求
    strcpy(buf, _501msg);
  }
  return buf;
}
int close_socket(int sock)
{
  if (close(sock))
  {
    fprintf(stderr, "Failed closing socket.\n");
    return 1;
  }
  return 0;
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

    while ((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
    {
      // 进行解析和发送//buf存储收到信息
      Request *request = parse(buf, readret, 8192);
      // 根据请求方法处理

      handle_request(request, buf);

      int len = strlen(buf);
      if (send(client_sock, buf, len, 0) != len)
      {
        close_socket(client_sock);
        close_socket(sock);
        fprintf(stderr, "Error sending to client.\n");
        return EXIT_FAILURE;
      }
      memset(buf, 0, BUF_SIZE);
    }

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
