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

typedef enum _Algorithms {SJFSD, SJFCD, HRRN} _Algorithm;

typedef struct
{
    int id;
    int len;
    int len2;
} __attribute__((packed)) content_header;

typedef struct
{
    int socket;
    char* name;
    int instructions_counter;
    float cpu_time_estimated;
} t_esi;

typedef struct
{
    t_queue * cola_esis_bloqueados;
    char* key;
} clave_bloqueada_t;

int stop; //variables globales
t_log * logger;
t_config * config;
_Algorithm algorithm;
int listeningPort;
int initial_estimation;
int alpha;
int socket_c;
char* ip_c;
char* port_c;
t_esi* un_esi;
t_list * lista_ready; //para no marear lo pongo en español
t_list * lista_bloqueados;
pthread_mutex_t pause_mutex;
sem_t esi_executing;
sem_t coordinador_pregunta;
sem_t esi_respuesta;

void exit_with_error(t_log* logger, char* error_message); //shared library
void configure_logger();
void get_values_from_config(t_log* logger, t_config* config);
void get_int_value(t_log* logger, char* key, int *value, t_config* config); //shared library
void get_string_value(t_log* logger, char* key, char* *value, t_config* config); //shared library
void new_blocked_keys();
void HostConnections(/*void * parameter*/);
int connect_to_server(char * ip, char * port, char *server); //shared library
void send_header(int socket, int id); //shared library
void wait_question(int socket);
void check_key(char* key);
void unlock_key(char* key);
void update_values();
void calculate_estimation(t_esi* otro_esi);
//void block_esi(t_esi * un_esi, clave_bloqueada_t* a_key);
void send_esi_to_ready(t_esi * un_esi);
void finish_esi(t_esi * un_esi);
_Algorithm to_algorithm(char* string);

#endif /* PLANIFICADOR_H_ */
