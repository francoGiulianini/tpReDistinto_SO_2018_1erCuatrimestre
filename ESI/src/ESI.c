/*
 ============================================================================
 Name        : ESI.c
 Author      : 
 Version     :
 Copyright   : Los Mas Mejores ©
 Description : Script en C, Ansi-style
 ============================================================================
 */

#include "ESI.h"

t_log * logger;
#define IP_S "127.0.0.1"
#define PORT_S "8080"
#define IP_C "127.0.0.1"
#define PORT_C "8081"

int main(void) 
{
	configure_logger();
	int scheduler_socket = connect_to_server(IP_S, PORT_S, "Scheduler");
	send_hello(scheduler_socket);
	int coordinator_socket = connect_to_server(IP_C, PORT_C, "Coordinator");
	send_hello(coordinator_socket);	
	while(1);
	return EXIT_SUCCESS;
}

void configure_logger()
{
  logger = log_create("ESI.log", "ESI", true, LOG_LEVEL_INFO);
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

void  send_hello(int socket) 
{
	char * header = "20";

	int result = send(socket, header, 3, 0);
	if (result <= 0)
		log_error(logger, "cannot send hello");
		//exit_with_error(logger, "Cannot send Hello");
	free(header);
}