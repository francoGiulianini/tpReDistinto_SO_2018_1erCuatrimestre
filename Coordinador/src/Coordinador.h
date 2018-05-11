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
#include <commons/string.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define MAX_INSTANCES 5

typedef enum _Clients {INSTANCE, ESI, SCHEDULER} _Client;
typedef enum _Algorithms {EL, LRU, KE} _Algorithm;

struct addrinfo hints;
struct addrinfo *serverInfo;

typedef struct
{
    int id;
    int len;
} __attribute__((packed)) content_header;

typedef struct
{
    char* key;
    char* value;
} __attribute__((packed)) message_content;

typedef struct
{
    char* name;
    int socket;
    int times_used; //para equitative load
    int space_used; //para least space used
    int is_active; //para saber si se cayo o no
    sem_t start;
} instance_t;

typedef struct 
{
    int next_message_len;
    int socket;
} thread_data_t;

t_log * logger;
int listeningPort;
_Algorithm algorithm;
struct sockaddr_in serverAddress;
_Client hello_id;
t_list * instances;
message_content* message;
//instance_t instances[MAX_INSTANCES];

void configure_logger();
void get_config_values(t_config* config);
void exit_with_error(t_log* logger, char* error_message);
void create_server();
void host_connections();
void host_instance(void* arg);
void host_esi(void* arg);
void host_scheduler(void* arg);
void process_message_header(content_header* header, int socket);
void assign_instance(_Algorithm algorithm, t_list* instances);
void disconnect_socket(int socket, bool is_instance);
instance_t* add_instance_to_list(char* name, int socket);
void disconnect_instance_in_list(int socket);
instance_t* name_is_equal(t_list* lista, char* name);
instance_t* socket_is_equal(t_list* lista, int socket);
instance_t* find_by_times_used(t_list* lista);
_Algorithm to_algorithm(char* string);

#endif /* COORDINADOR_H_ */
