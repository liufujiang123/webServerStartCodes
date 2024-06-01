#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

void ErrorLog(char *, struct sockaddr_in, int);
void AccessLog(char *, struct sockaddr_in, char *, int, int);