#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <commons/log.h>
#include <commons/config.h>

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

t_log * logger;
t_config * config;
configuracion_t cofiguracion;

void configure_logger();
void exit_with_error(t_log* logger, char* error_message);
void get_values_from_config(t_log* logger, t_config* config);
void get_int_value(t_log* logger, char* key, int *value, t_config* config);
void get_string_value(t_log* logger, char* key, char* *value, t_config* config);
int connect_to_server(char * ip, char * port, char * server);
void  send_hello(int socket); 

#endif
