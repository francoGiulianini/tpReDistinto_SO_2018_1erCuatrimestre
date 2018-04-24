#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <commons/log.h>

#define WELCOME_MSG 8

void configure_logger();
int connect_to_server(char * ip, char * port, char * server);

#endif