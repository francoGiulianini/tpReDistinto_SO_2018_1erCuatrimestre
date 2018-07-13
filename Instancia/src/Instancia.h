#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#define WELCOME_MSG 8
#define FILE_SIZE 1024 //en bytes

typedef struct
{
    int id;
    int lenClave;
    int lenValor;
} __attribute__((packed)) content_header;

typedef struct
{
    char* clave;
    char* valor;
}__attribute__((packed)) content;

typedef struct
{
    int cantEntradas;
    int tamanioEntradas;
}__attribute__((packed)) configuracion_t;

typedef struct
{
    char clave[40];
    int tamanio;
	int age;
} entrada_t;

typedef struct
{
    char* clave;
    char* map;
    int fd;
} map_t;

t_log * logger;
t_config * config;
configuracion_t * configuracion;
int coordinator_socket;
t_list* lista_claves;
sem_t semCompactar;

void configure_logger();
void exit_with_error(t_log* logger, char* error_message);
void get_values_from_config(t_log* logger, t_config* config);
void get_int_value(t_log* logger, char* key, int *value, t_config* config);
void get_string_value(t_log* logger, char* key, char* *value, t_config* config);
int connect_to_server(char * ip, char * port, char * server);
void  send_hello(int socket);
void recibirTamanos();
int consultarTabla (entrada_t* tabla, char* clave);
void guardarEnTabla (entrada_t* tabla, content* mensaje, int posicion);
void guardarEnMem (content* mensaje, int posicion);
int revisarLista(char* clave);
void guardarEnClaves(content_header* header, char* clave);
void procesarHeader (content_header* header, entrada_t* tabla);
void send_header(int socket, int id);
map_t * buscar_por_clave(t_list* lista_claves, char* clave);
void compactar (entrada_t * tabla);
int getCantPaginas (int tamanioMensaje );
int consultarTablaCIRC (entrada_t* tabla, content* mensaje);
int consultarTablaLRU (entrada_t* tabla, content* mensaje, int* laMasVieja);
void guardarEnTablaCIRC(entrada_t * tabla, content* mensaje, int* cantPaginas);
void guardarEnTablaLRU(entrada_t * tabla, content* mensaje, int* cantPaginas,int* laMasVieja);


#endif
