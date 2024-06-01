#include "log.h"

void ErrorLog(char *msg, struct sockaddr_in cli_addr, int fd)
{
  time_t now;
  time(&now);
  char *Time = ctime(&now);
  Time[strlen(Time) - 1] = '\0';
  printf("\033[0m\033[1;31m[%s] [Error] [Client ip%12s:%-7d] [client fd %d] %s\033[0m\n",
         Time,
         inet_ntoa(cli_addr.sin_addr),
         (int)ntohs(cli_addr.sin_port),
         fd,
         msg);
}

void AccessLog(char *msg, struct sockaddr_in cli_addr, char *method, int code, int fd)
{
  time_t now;
  time(&now);
  char *Time = ctime(&now);
  Time[strlen(Time) - 1] = '\0';
  printf("\033[0m\033[1;32m[%s] [Access] [Client ip%12s:%-7d] [client fd %d] %s %d %s\033[0m\n",
         Time,
         inet_ntoa(cli_addr.sin_addr),
         (int)ntohs(cli_addr.sin_port),
         fd,
         method,
         code,
         msg);
}