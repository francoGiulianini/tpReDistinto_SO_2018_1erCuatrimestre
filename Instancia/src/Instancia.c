/*
 ============================================================================
 Name        : Instancia.c
 Author      : 
 Version     :
 Copyright   : Los Mas Mejores ©
 Description : Guardar Claves en Memoria y Disco en C, Ansi-style
 ============================================================================
 */

#include "Instancia.h"

char* port_c;
char* ip_c;
char* name;
int noHayLugar;

int main(void)
{

	configure_logger();
	config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");
    get_values_from_config(logger, config);
	int coordinator_socket = connect_to_server(ip_c, port_c, "Coordinator");
	send_hello(coordinator_socket);
	
	recibirTamanos();

	send(coordinator_socket, name, strlen (name), 0);	

	entradas_t tabla[configuracion.cantEntradas];

	char* mem = malloc(sizeof (char) *configuracion.cantEntradas*configuracion.tamanioEntradas);
	while(1)
	{
	content_header *header = malloc (sizeof (content_header));

	recv(coordinator_socket, header, sizeof (content_header), 0);
	procesarHeader (header);
	
	}
	return EXIT_SUCCESS;
}

void configure_logger()
{
  logger = log_create("Instancia.log", "Instancia", true, LOG_LEVEL_INFO);
}

void exit_with_error(t_log* logger, char* error_message)
{
    log_error(logger, error_message);
    log_destroy(logger);
    exit(EXIT_FAILURE);
}

void get_values_from_config(t_log* logger, t_config* config)
{
    get_string_value(logger, "PuertoCoordinador", &port_c, config);
    get_string_value(logger, "IPCoordinador", &ip_c, config);
	get_string_value(logger, "NombreInstancia", &name, config);
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

void  send_hello(int socket) 
{
	content_header * header_c = (content_header *) malloc (sizeof(content_header));
	
	header_c->id=10;
	header_c->len=strlen(name);

	int result = send(socket, header_c, sizeof(content_header), 0);
	if (result <= 0)
		exit_with_error(logger, "Cannot send Hello");
}
void recibirTamanos ()
{
	configuracion = malloc(sizeof(configuracion_t));

	recv
}


// funciones auxiliares, ignorelas

int lenEnBloques (lenMensaje)
float aux = lenMensaje / lenBloqueDeMem
if (aux / 


int consultarTabla (entrada_t tabla, content mensaje, int tamanioMensaje){

	int cantPaginas = 0;
	if (tamanioMensaje % configuracion.tamanioEntradas == 0){
		cantPaginas = tamanioMensaje div confiuracion.tamanioEntradas
	} else{
		cantPaginas = (tamanioMensaje div confiuracion.tamanioEntradas) + 1;
	}
	for (int i = 0; i <= configuracion.cantEntradas; i++){
		if (int lugarVacio = string_equals_ignore_case(tabla[i].clave, "vacio")){
			for (int j = i; j < i + cantPaginas; j++){
				lugarVacio = string_equals_ignore_case(tabla[i].clave, "vacio")
			}
			if (lugarVacio) {return i;}
		}
	}
	return -1;

}

void guardarEnTabla (entrada_t tabla,content mensaje, int posicion){

	int cantPaginas = 0;
	if (tamanioMensaje % configuracion.tamanioEntradas == 0){
		cantPaginas = tamanioMensaje div confiuracion.tamanioEntradas
	} else{
		cantPaginas = (tamanioMensaje div confiuracion.tamanioEntradas) + 1;
	}
	for (i = posicion; i <= cantPaginas; i++){
		memcpy (tabla[i].clave , mensaje.clave , strlen(mensaje.clave));
	}
}


void guardarEnMem (char* mem, content mensaje, int posicion){
	int ubicacionEnMem = posicion * configuracion.tamanioEntradas;
	memcpy (mem + ubicacionEnMem , mensaje.valor , strlen(mensaje.valor));
}


void procesarHeader (content_header header){
	switch header->id{
		case 11 : {
			// compactar
		}

		case 12 : {
			content *mensaje = malloc (header->lenClave + header->lenValor);

			recv(coordinator_socket, mensaje, sizeof (header->lenClave + header->lenValor), 0);

			int posicion = consultarTabla (tabla, mensaje, header->lenValor);

			if (noHayLugar){
				send(coordinator_socket, 11, sizeof(int), 0); //hay que compactar

				//activar semaforo 

				int posicion = consultarTabla (mensaje);

			}

			guardarEnTabla (tabla,mensaje,posicion);
			guardarEnMem (mem,mensaje,posicion);


			send(coordinator_socket, 12, sizeof(int), 0);

			free(header->lenClave);
			free(header->lenValor);
			free(header);
			free(mensaje);
		}
		case 13:{ //GET
			//ver mmap
		}
	}

}