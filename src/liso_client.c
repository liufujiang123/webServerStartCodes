/******************************************************************************
 * echo_client.c                                                               *
 *                                                                             *
 * Description: This file contains the C source code for an echo client.  The  *
 *              client connects to an arbitrary <host,port> and sends input    *
 *              from stdin.                                                    *
 *                                                                             *
 * Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
 *          Wolf Richter <wolf@cs.cmu.edu>                                     *
 *                                                                             *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "usage: %s <server-ip> <port>", argv[0]);
    return EXIT_FAILURE;
  }

  char buf[BUF_SIZE];

  int status, sock;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  struct addrinfo *servinfo;       // will point to the results
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s \n", gai_strerror(status));
    return EXIT_FAILURE;
  }

  if ((sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
  {
    fprintf(stderr, "Socket failed");
    return EXIT_FAILURE;
  }

  if (connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
  {
    fprintf(stderr, "Connect");
    return EXIT_FAILURE;
  }

  char msg[BUF_SIZE];
  while (1)
  {
    char *find;
    fgets(msg, BUF_SIZE, stdin);
    find = strchr(msg, '\n');
    if (find) // 濡傛灉find涓嶄负绌烘寚閽?
      *find = '\0';
    int fd_in = open(msg, O_RDONLY);
    if (fd_in < 0)
    {
      printf("Failed to open the file\n");
      return 0;
    }
    read(fd_in, msg, 8192);

    fprintf(stdout, "-----------------------Sending-----------------------\n%s", msg);

    send(sock, msg, strlen(msg), 0);
    printf("Request success.\n");
    printf("Waiting for response.\n");
    int bytes_received;
    if ((bytes_received = recv(sock, buf, BUF_SIZE, 0)) > 1)
    {
      buf[bytes_received] = '\0';
      fprintf(stdout, "-----------------------Received-----------------------\n%s", buf);
    }
    // 鎻愪氦
    memset(msg, 0, BUF_SIZE);
  }

  freeaddrinfo(servinfo);
  close(sock);
  return EXIT_SUCCESS;
}