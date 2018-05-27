/*
 ============================================================================
 Name        : ESI.c
 Author      : Los M�s Mejores (2018)
 Version     :
 Copyright   : Los Mas Mejores ©
 Description : Script en C, Ansi-style
 ============================================================================
 */

#include "ESI.h"

#include <parsi/parser.h>

char * ip_s;
char * port_s;
char * ip_c;
char * port_c;
FILE * script;
ssize_t read;
char * line = NULL;
t_esi_operacion parsed;
size_t len = 0;
bool flag_blocked = false;
int scheduler_socket;
int coordinator_socket;

int main(int argc, char* argv[]) 
{
	configure_logger();

	check_arguments(argc);

	script = fopen(argv[1], "rb");
	if(script == NULL)
        exit_with_error(logger, "Cannot open script");

	config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");

    get_values_from_config(logger, config);

	scheduler_socket = connect_to_server(ip_s, port_s, "Scheduler");

	send_hello(scheduler_socket);

	coordinator_socket = connect_to_server(ip_c, port_c, "Coordinator");
	
	content_header * buffer = (content_header*) malloc(sizeof(content_header));

	while(1)
	{
		recv(scheduler_socket, buffer, sizeof(content_header), 0);
		if (!flag_blocked) parsed = parse_line();
		send_parsed_operation(&parsed, &flag_blocked);
	}

	free(buffer);
	return EXIT_SUCCESS;
}

void configure_logger()
{
  logger = log_create("ESI.log", "ESI", true, LOG_LEVEL_INFO);
}

void check_arguments(int argc)
{
	switch (argc)
	{
		case 1:
			exit_with_error(logger, "Missing script argument");
			break;

		case 2:
			break;

		default:
			exit_with_error(logger, "Too many arguments!");
	}
}

void exit_with_error(t_log* logger, char* error_message)
{
    log_error(logger, error_message);
    log_destroy(logger);
    exit(EXIT_FAILURE);
}

void get_values_from_config(t_log* logger, t_config* config)
{
    get_string_value(logger, "PuertoPlanificador", &port_s, config);
	get_string_value(logger, "IPPlanificador", &ip_s, config);
    get_string_value(logger, "PuertoCoordinador", &port_c, config);
    get_string_value(logger, "IPCoordinador", &ip_c, config);
}

void get_int_value(t_log* logger, char* key, int *value, t_config* config)
{
    if(config_has_property(config, key))
    {
        *value = config_get_int_value(config, key);
        log_info(logger, "%s from config file: %d", key, *value);
    }
    else
    {
        log_error(logger, "Config does not contain %s", key);
        exit_with_error(logger, "");
    }
}

void get_string_value(t_log* logger, char* key, char* *value, t_config* config)
{
    if(config_has_property(config, key))
    {
        *value = config_get_string_value(config, key);
        log_info(logger, "%s from config file: %s", key, *value);
    }
    else
    {
        log_error(logger, "Config does not contain %s", key);
        exit_with_error(logger, "");
    }
}

int connect_to_server(char * ip, char * port, char *server)
{
	struct addrinfo hints;
  	struct addrinfo *server_info;

  	memset(&hints, 0, sizeof(hints));
  	hints.ai_family = AF_UNSPEC; 
  	hints.ai_socktype = SOCK_STREAM; 

  	getaddrinfo(ip, port, &hints, &server_info);  // Cambiar IP y PORT por los de archivo de config

  	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

  	int res = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

  	freeaddrinfo(server_info);

  	if (res < 0) 
  	{
   		log_error(logger, "Cannot connect to: %s", server);
		return -1;
  	}
  
  	log_info(logger, "Connected with the: %s", server);

	char * buffer = (char*) calloc(sizeof(char), WELCOME_MSG);

	recv(server_socket, buffer, WELCOME_MSG, 0);

	log_info(logger, "%s", buffer);
	free(buffer);

	return server_socket;
}

void  send_message(int socket, int id, char * message1, char * message2) 
{
	content_header * header_c = (content_header*) malloc(sizeof(content_header));

	int len_message1 = strlen(message1);
	int len_message2 = strlen(message2);
    header_c->id = id;
    header_c->len = len_message1;
	header_c->len2 = len_message2;

	int result = send(socket, header_c, sizeof(content_header), 0);
	t_message * message = (t_message *) malloc ((len_message1 + len_message2) * sizeof(char)); //PUEDE llegar a romper
	message->key = message1;
	message->value = message2;
	
	free(header_c);
	
	if (result <= 0)
		exit_with_error(logger, "Cannot send Header for Message: %s, with instruction %d", message, id);
	
	if (len_message > 0)
		result = send(socket, message, (len_message1 + len_message2), 0);
	
	if (result <= 0)
		exit_with_error(logger, "Cannot send Payload for Message: %s, with instruction %d" message, id);
}

void send_parsed_operation(t_esi_operacion parsed, bool bloqueado)
{
	content_header * buffer = (content_header*) malloc(sizeof(content_header));
	int result;

	if(parsed.valido){

		switch(parsed.keyword){
			case GET:
				send_message(coordinator_socket, 21, parsed.argumentos.GET.clave, "");
				break;
			case SET:
				send_message(coordinator_socket, 22, parsed.argumentos.SET.clave, parsed.argumentos.SET.valor); // el ESI tambien manda el valor
				break;
			case STORE:
				send_message(coordinator_socket, 23, parsed.argumentos.STORE.clave, "");
				break;
			default:
				exit_with_error(logger, "unable to interprete line from script");
		}

	result = recv(coordinator_socket, buffer, sizeof(content_header), 0);
	switch (result){
		case -1:
			exit_with_error(logger, "Cannot receive expected answer from Coordinator");
			break;
		case 0:
			exit_with_error(logger, "Coordinator Disconnected");
			break;
		default:
			break;
	}

		switch (buffer->id)
		{
			case 31:
				send_message(scheduler_socket, 22, "", "");
				destruir_operacion(parsed);
				break;
			case 32:
				bloqueado = true;
				send_message(scheduler_socket, 23, "", "");
				break;
			default:
				exit_with_error(logger, "Message type not valid: %d", buffer->id);
		}
		free(buffer);
	}
	else
	{
		exit_with_error(logger, "line from script is not valid");
	}

	if(feof(script)) exit(EXIT_SUCCESS);
}

t_esi_operacion parse_line() {
	if ((read = getline(&line, &len, script)) != -1)
	{
		t_esi_operacion parsed = parse(line);
		return parsed;
	}
	else
	{
			exit_with_error("unable to read line from script");
	}
}
