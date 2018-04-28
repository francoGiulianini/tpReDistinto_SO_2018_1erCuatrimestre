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

struct addrinfo hints;
struct addrinfo *serverInfo;
int listeningSocket;

#endif /* COORDINADOR_H_ */
