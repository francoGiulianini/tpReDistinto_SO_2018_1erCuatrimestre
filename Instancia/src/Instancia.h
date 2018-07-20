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
#include <limits.h>     /* PATH_MAX */
#include <sys/stat.h>   /* mkdir(2) */
#include <errno.h>

#define WELCOME_MSG 8

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

typedef struct nodo{
	char* dato;
	struct nodo *siguiente;
} nodo_t;
   
typedef nodo_t *ptrNodo;
typedef nodo_t *ptrLista;

t_log * logger;
t_config * config;
configuracion_t * configuracion;
int coordinator_socket;
t_list* lista_claves;
//ptrLista listaGETs;
pthread_mutex_t mutex_dump;

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
void send_header_with_length(int socket, int id, int len1, int len2);
int free_entries(entrada_t * tabla);
map_t * buscar_por_clave(t_list* lista_claves, char* clave);
void compactar (entrada_t * tabla);
int getCantPaginas (int tamanioMensaje);
int consultarTablaCIRC (entrada_t* tabla, content* mensaje);
int consultarTablaLRU (entrada_t* tabla, content* mensaje, int* laMasVieja);
int buscarEspacioLibre (entrada_t*tabla, content* mensaje, int cantPaginas);
void guardarEnTablaCIRC(entrada_t * tabla, content* mensaje, int cantPaginas);
void guardarEnTablaLRU(entrada_t * tabla, content* mensaje, int cantPaginas,int* laMasVieja);
void storeKey(entrada_t * tabla, char* clave);
void dump(entrada_t * tabla);
ptrNodo crearNodo(char* clave);
void insertarNodo(ptrLista* lista, char* clave);
int estaEnLaLista(ptrNodo listaGETs, char* clave);
int mkdir_p(const char *path);

#endif
