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
#include <sys/mman.h>

char* port_c;
char* ip_c;
char* name;
int noHayLugar;
char * msjAlCoordinador = ""

int main(void)
{

	configure_logger();
	config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");
    get_values_from_config(logger, config);
	coordinator_socket = connect_to_server(ip_c, port_c, "Coordinator");
	send_hello(coordinator_socket);
	
	send(coordinator_socket, name, strlen (name), 0);
	
	recibirTamanos();

	/* inicializamos la tabla */
	entrada_t tabla[configuracion->cantEntradas];
	for ( i = 0; i = configuracion->cantEntradas -1 ; i++){
		tabla[i].clave = "vacio"
	}
	
	char* mem = malloc(sizeof (char) *configuracion->cantEntradas*configuracion->tamanioEntradas);
	while(1)
	{
		content_header *header = malloc (sizeof (content_header));

		recv(coordinator_socket, header, sizeof (content_header), 0);
		procesarHeader (header, &tabla, &mem);

		free(header->lenClave);
		free(header->lenValor);
		free(header);
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
	header_c->lenClave=strlen(name);

	int result = send(socket, header_c, sizeof(content_header), 0);
	if (result <= 0)
		exit_with_error(logger, "Cannot send Hello");
}
void recibirTamanos ()
{
	configuracion = (configuracion_t*)malloc(sizeof(configuracion_t));

	recv(coordinator_socket, configuracion, sizeof(configuracion), 0);
}

int consultarTabla (entrada_t* tabla, content* mensaje, int tamanioMensaje){

	int cantPaginas = 0;
	int lugarVacio = 0;

	//deberia fijarse si la clave ya existe en la tabla
	//y devolver esa posicion

	if (tamanioMensaje % configuracion->tamanioEntradas == 0){
		cantPaginas = tamanioMensaje / configuracion->tamanioEntradas;
	} else{
		cantPaginas = (tamanioMensaje / configuracion->tamanioEntradas) + 1;
	}
	for (int i = 0; i <= configuracion->cantEntradas; i++){
		if ((lugarVacio = string_equals_ignore_case(tabla[i].clave, "vacio")) || string_equals_ignore_case(tabla[i].clave, mensaje->clave)){
			for (int j = i; j < i + cantPaginas; j++){
				lugarVacio = ( string_equals_ignore_case(tabla[i].clave, "vacio") || string_equals_ignore_case(tabla[i].clave, mensaje->clave));
			}
			if (lugarVacio) {return i;}
		}
	}
	return -1;

}

void guardarEnTabla (entrada_t* tabla,content* mensaje, int posicion){

	int cantPaginas = 0;
	int tamanioMensaje = strlen(mensaje->clave);
	if (tamanioMensaje % configuracion->tamanioEntradas == 0){
		cantPaginas = tamanioMensaje / configuracion->tamanioEntradas;
	} else{
		cantPaginas = (tamanioMensaje / configuracion->tamanioEntradas) + 1;
	}
	for (int i = posicion; i <= cantPaginas; i++){
		memcpy (tabla[i].clave , mensaje->clave , strlen(mensaje->clave));
		//guardar en tabla el tamaño del valor
	}
	//actualizar archivo de clave?
}

void guardarEnMem (char* mem, content* mensaje, int posicion){
	int ubicacionEnMem = posicion * configuracion->tamanioEntradas;
	memcpy (mem + ubicacionEnMem , mensaje->valor , strlen(mensaje->valor));
	//actualizar archivo de clave?
}

void procesarHeader (content_header* header, entrada_t* tabla, char* mem){
	switch (header->id){
		case 11 : {
			// compactar
			// circular
			for i = 
		}

		case 12 : { //SET

			
			content *mensaje = malloc (header->lenClave + header->lenValor);

			recv(coordinator_socket, mensaje, sizeof (header->lenClave + header->lenValor), 0);

			int posicion = consultarTabla (tabla, mensaje, header->lenValor);	

			if (noHayLugar){

				msjAlCoordinador = "compactar"
				send(coordinator_socket, msjAlCoordinador, sizeof(int), 0); //hay que compactar

				//activar semaforo 

				int posicion = consultarTabla (mensaje, tabla, strlen(mensaje->valor));

			}

			guardarEnTabla (&tabla,mensaje,posicion);
			guardarEnMem (&mem,mensaje,posicion);

			msjAlCoordinador = "SET_OK"
			send(coordinator_socket, msjAlCoordinador, sizeof(int), 0);

			free(mensaje);
		}
		case 13:{ //GET
			//crear archivo con la clave que tenga la posicion en la tabla y el valor
			//ver mmap


			int result;
			char *map;
			char nombreDeClave[40] = content->clave;

			int fd = open(nombreDeClave, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
			if (fd == -1) {
				perror("Error al intentar abrir el archivo");
				exit(EXIT_FAILURE);
    		}

			result = lseek(fd, content_header->lenValor-1, SEEK_SET);
			if (result == -1) {
				close(fd);
				perror("Error en lseek()");
				exit(EXIT_FAILURE);
			}
			result = write(fd, "", 1);
			if (result != 1) {
				close(fd);
				perror("Error al escribir el ultimo byte del archivo");
				exit(EXIT_FAILURE);
			}

			map = mmap(NULL, content_header->lenValor, PROT_WRITE, MAP_SHARED, fd, NULL);
			if (map == MAP_FAILED) {
				close(fd);
				perror("Error la mapear el archivo");
				exit(EXIT_FAILURE);
			}

			//aca hay que escribir el archivo			
			memcpy(map, content->valor, content_header->lenValor);
			memcpy (mem + ubicacionEnMem , mensaje->valor , strlen(mensaje->valor));


			/* A esta parte la dejo comentada por ahora porque no estoy seguro de donde habria que leberar los recursos, sepues lo acomodo

			if (munmap(map, content_header->lenValor) == -1) {
			perror("Error un-mmapping the file");

			}

   			 close(fd);
			*/

		}
	}

}