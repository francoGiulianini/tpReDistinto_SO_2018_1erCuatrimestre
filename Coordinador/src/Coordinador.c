/*
 ============================================================================
 Name        : Coordinador.c
 Author      : 
 Version     :
 Copyright   : Los Mas Mejores ©
 Description : Coordina las instrucciones del ESI en C, Ansi-style
 ============================================================================
 */

#include "Coordinador.h"

t_config* config;
int listeningPort;
#define MAX_LISTENING_CONNECTIONS 5

int main(void) {
	
	configure_logger();

    config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");							/*Logica repetida*/

    if(config_has_property(config, "PuertoEscucha"))
        {
            listeningPort = config_get_int_value(config, "PuertoEscucha");
            log_info(logger, "Port from config file: %d", listeningPort);
        }
    else
        exit_with_error(logger, "Cannot read port from config file");
        
    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	
	getaddrinfo(NULL, "8081"/*listeningPort*/, &hints, &serverInfo);
	listeningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
	bind(listeningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
	listen(listeningSocket, MAX_LISTENING_CONNECTIONS);
	
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int socketESI = accept(listeningSocket, (struct sockaddr *) &addr, &addrlen);					/*	Hasta aca estar�a escuchando al ESI	*/
	
	send(socketESI, "WELCOME", 8, 0);

	/*char * header = malloc(3);*/
	content_header * header_c = (content_header *) malloc(sizeof(content_header));

	int recv_result = recv(socketESI, header_c, sizeof(content_header), 0);
	if(recv_result <= 0)
	{
		perror("recv");
		exit_with_error(logger, "Cannot receive message");
	}
	if(header_c->id == 10)
	{
		log_info(logger,"Connected with an Instance");
		//agregar a vector de instancias
	}
	if(header_c->id == 20)
	{
		log_info(logger,"Connected with an ESI");
		//agregar a vector de esis
	}
	if(header_c->id == 30)
	{
		log_info(logger,"Connected with the Scheduler");
		//agregar a una variable
	}

	free(header_c);
	/*	Falta hacer todos los chequeos de errores para el log, la conexi�n con las instancias,
	 *	y todo el manejo de la informacion que manda el ESI
	 */
	
	return EXIT_SUCCESS;
}

void exit_with_error(t_log* logger, char* error_message)
{
    log_error(logger, error_message);
    log_destroy(logger);
    exit(EXIT_FAILURE);
}

void configure_logger()
{
  logger = log_create("Coordinador.log", "Coordinador", true, LOG_LEVEL_INFO);
}
