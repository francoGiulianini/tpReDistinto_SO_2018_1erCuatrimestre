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
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>

/*#define GET 31
#define STORE 32
#define SET 33
#define Abort 34
#define STATUS 39*/
#define MAX_KEY_LENGTH 40

typedef enum _Operations {GET, SET, STORE, STATUS, ABORT, COMPACT} _Operation;
typedef enum _Clients {INSTANCE, ESI, SCHEDULER} _Client;
typedef enum _Algorithms {EL, LSU, KE} _Algorithm;

struct addrinfo hints;
struct addrinfo *serverInfo;

typedef struct
{
    int id;
    int len;
    int len2;
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
    int free_space; //para least space used
    int is_active; //para saber si se cayo o no
	char letter_min;
	char letter_max; //para key explicit
    sem_t start;
    t_dictionary* keys;
} instance_t;

typedef struct
{
    int number;
    int size;
} __attribute__((packed)) config_instance_t;

typedef struct 
{
    int next_message_len;
    int socket;
} thread_data_t;

t_log * logger;
t_config* config;
int listeningPort;
_Algorithm algorithm;
int delay;
int max_storage_size;
struct timespec delay_req, time_rem;
struct sockaddr_in serverAddress;
_Client hello_id;
t_list * instances;
content_header * header_c;
message_content* message;
config_instance_t* config_instance;
sem_t one_instance;
sem_t one_esi;
sem_t esi_operation;
sem_t scheduler_response;
sem_t result_instance;
sem_t compact;

void configure_logger();
void get_config_values(t_config* config);
void exit_with_error(t_log* logger, char* error_message);
void create_server();
void host_connections();
void host_instance(void* arg);
void host_esi(void* arg);
void host_scheduler(void* arg);
void process_message_header(content_header* header, int socket);
void process_message_header_esi(content_header* header, int socket, t_dictionary * blocked_keys, char* name);
void operation_get(content_header* header, int socket, t_dictionary * blocked_keys, char* name);
void operation_set(content_header* header, int socket, t_dictionary * blocked_keys, char* name);
void operation_store(content_header* header, int socket, t_dictionary * blocked_keys, char* name);
void operation_status(content_header* header, int socket);
void initiate_compactation(int socket);
void abort_esi(int socket, int stage);
void send_header(int socket, int id);
bool test_connection(int socket);
instance_t* simulate_assignment();
void assign_instance(_Algorithm algorithm, t_list* instances);
instance_t* choose_instance(bool simulate);
int save_on_instance(t_list* instances);
void disconnect_socket(int socket, bool is_instance);
instance_t* add_instance_to_list(char* name, int socket);
void update_instance(instance_t* one_instance, content_header* header);
void assign_letters();
void disconnect_instance_in_list(int socket);
instance_t* name_is_equal(t_list* lista, char* name);
instance_t* socket_is_equal(t_list* lista, int socket);
instance_t* choose_by_counter(t_list* lista, bool simulate);
instance_t* choose_by_space(t_list* lista, bool simulate);
instance_t* choose_by_letter(t_list* lista);
instance_t* find_by_key(t_list* lista, char* key);
instance_t* find_by_key_and_active(t_list* lista, char* key);
t_list* find_by_free_space(t_list* lista);
instance_t* find_by_letter(t_list* lista, char letter);
_Algorithm to_algorithm(char* string);

#endif /* COORDINADOR_H_ */
