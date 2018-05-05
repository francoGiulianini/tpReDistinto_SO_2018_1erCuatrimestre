/*
 * Coordinador.h
 *
 *  Created on: 27 abr. 2018
 *      Author: utnso
 */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10

struct addrinfo hints;
struct addrinfo *serverInfo;

typedef struct
{
    int id;
    int len;
} __attribute__((packed)) content_header;

t_log * logger;
int listeningSocket;
struct sockaddr_in serverAddress;

void configure_logger();
void exit_with_error(t_log* logger, char* error_message);
void HostConnections();

#endif /* COORDINADOR_H_ */
