#ifndef ESI_H_
#define ESI_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <commons/log.h>
#include <commons/config.h>
#include <parsi/parser.h>

#define WELCOME_MSG 8

typedef struct
{
    int id;
    int len;
    int len2;
} __attribute__((packed)) content_header;

t_config* config;
t_log * logger;

typedef struct
{
	char * key;
	char * value;
} __attribute__((packed)) t_message;

void configure_logger();
void check_arguments(int argc);
void exit_with_error(t_log* logger, char* error_message);
void get_values_from_config(t_log* logger, t_config* config);
void get_int_value(t_log* logger, char* key, int *value, t_config* config);
void get_string_value(t_log* logger, char* key, char* *value, t_config* config);
int connect_to_server(char * ip, char * port, char * server);
void send_message(int socket, int id, char * message1, char * message2);
void send_parsed_operation(t_esi_operacion parsed, bool bloqueado);
t_esi_operacion parse_line();


#endif
