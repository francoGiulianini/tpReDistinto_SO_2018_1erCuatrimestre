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

struct addrinfo hints;
struct addrinfo *serverInfo;

t_log * logger;
int listeningSocket;

void configure_logger();
void exit_with_error(t_log* logger, char* error_message);

#endif /* COORDINADOR_H_ */
