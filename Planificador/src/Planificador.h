/*
 * Planificador.h
 *
 *  Created on: 19 abr. 2018
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

#define WELCOME_MSG 8

int stop; //variables globales
t_log * logger;

void exit_with_error(t_log* logger, char* error_message);
void configure_logger();
void get_values_from_config(t_log* logger, t_config* config);
void get_int_value(t_log* logger, char* key, int *value, t_config* config);
void get_string_value(t_log* logger, char* key, char* *value, t_config* config);
void *HostConnections(void * parameter);
int connect_to_server(char * ip, char * port, char *server); //hacer una shared library
void send_hello(int socket);

#endif /* PLANIFICADOR_H_ */
