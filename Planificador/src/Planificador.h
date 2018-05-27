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
#include <commons/collections/queue.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>

#define WELCOME_MSG 8
#define MAX_CLIENTS 2

typedef struct
{
    int id;
    int len;
} __attribute__((packed)) content_header;

typedef struct
{
    char* key;
    char* value;
} __attribute__((packed)) content_message;

typedef struct
{
    int socket;
    char* name;
    int instructions_counter;
} t_esi;

typedef struct
{
    t_queue * cola_esis_bloqueados;
    char* key;
} clave_bloqueada_t;

int stop; //variables globales
int block_esi;
t_log * logger;
t_config * config;
t_esi* un_esi;
t_queue * cola_ready; //para no marear lo pongo en español
t_list * lista_bloqueados;
pthread_mutex_t pause_mutex;
sem_t esi_listo;
sem_t coordinador_listo;
sem_t coordinador_pregunta;
sem_t esi_respuesta;

void exit_with_error(t_log* logger, char* error_message);
void configure_logger();
void get_values_from_config(t_log* logger, t_config* config);
void get_int_value(t_log* logger, char* key, int *value, t_config* config);
void get_string_value(t_log* logger, char* key, char* *value, t_config* config);
void new_blocked_keys();
void HostConnections(/*void * parameter*/);
int connect_to_server(char * ip, char * port, char *server); //hacer una shared library
void send_hello(int socket);
void send_header(int socket, int id);
void wait_question(int socket);
void wait_result(int socket);

#endif /* PLANIFICADOR_H_ */
