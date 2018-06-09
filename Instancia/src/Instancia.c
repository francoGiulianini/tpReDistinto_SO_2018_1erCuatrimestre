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
char* mem; //storage
//char * msjAlCoordinador = "";
pthread_t hiloCompactar;
int indexCircular = 0;

int main(void)
{
	sem_init(&semCompactar,0,0);
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
	for (int i = 0; i <= configuracion->cantEntradas; i++){
		strcpy(tabla[i].clave, "vacio");
	}

	lista_claves = list_create();
	
	mem = malloc(sizeof (char) *configuracion->cantEntradas*configuracion->tamanioEntradas);

	int error = pthread_create(&hiloCompactar, NULL, (void *)compactar, tabla);
			if(error != 0)
			{
				log_error(logger, "Couldn't create thread Compactar");
			}

	while(1)
	{
		content_header *header = malloc (sizeof (content_header));

		recv(coordinator_socket, header, sizeof (content_header), 0);
		procesarHeader (header, tabla);

		//free(header->lenClave);
		//free(header->lenValor);
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

void send_hello(int socket) 
{
	content_header * header_c = (content_header *) malloc (sizeof(content_header));
	
	header_c->id=10;
	header_c->lenClave= strlen(name);
	header_c->lenValor= 0;

	int result = send(socket, header_c, sizeof(content_header), 0);
	if (result <= 0)
		exit_with_error(logger, "Cannot send Hello");
}

void recibirTamanos ()
{
	configuracion = (configuracion_t*)malloc(sizeof(configuracion_t));

	recv(coordinator_socket, configuracion, sizeof(configuracion_t), 0);
	log_info(logger, "Received Number of Entries: %d", configuracion->cantEntradas);
	log_info(logger, "Received Size of Entries: %d", configuracion->tamanioEntradas);
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
			
			if (lugarVacio) {
				/*if (indexCircular < i + cantPaginas){ 
					indexCircular = i + cantPaginas;					
				}*/
				return i;				
			}
		}
	}
	/*if (indexCircular >= cantPaginas) {
					indexCircular = 0;
					posicion = 0;
				}*/
	return -1;

}

void guardarEnTabla (entrada_t* tabla, content* mensaje, int posicion){

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

void guardarEnMem (content* mensaje, int posicion){
	int ubicacionEnMem = posicion * configuracion->tamanioEntradas;
	memcpy (mem + ubicacionEnMem , mensaje->valor , strlen(mensaje->valor) + 1);
	//aca hay que escribir el archivo	
	//buscar en la lista la claves
	map_t* una_clave = buscar_por_clave(lista_claves, mensaje->clave);
	if(una_clave == NULL)
		log_error(logger, "ERROR");

	memcpy(una_clave->map, mensaje->valor, strlen(mensaje->valor) + 1);
}

int revisarLista(char* clave)
{
	/*bool _tiene_misma_clave(map_t* p)
	{
		return string_equals_ignore_case(p->clave, clave);
	} //esta funcion podria ser mejor
	return list_any_satisfy(lista_claves, _tiene_misma_clave);*/
	map_t* una_clave = buscar_por_clave(lista_claves, clave);
	if(una_clave == NULL)
		return 0;
	else
		return 1;
}

void guardarEnClaves(content_header* header, char* clave)
{
	//guardar en una tabla los maps
	int result;
	int fd = open(clave, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) {
		perror("Error al intentar abrir el archivo");
		exit(EXIT_FAILURE);
	}

	result = lseek(fd, FILE_SIZE/*definir un maximo? ->header->lenValor-1*/, SEEK_SET);
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

	map_t* una_clave = (map_t*)malloc(sizeof(map_t));
	una_clave->clave = malloc(header->lenClave + 1);
	memcpy(una_clave->clave, clave, header->lenClave + 1);
	una_clave->map = mmap(NULL, FILE_SIZE/*header->lenValor*/, PROT_WRITE, MAP_SHARED, fd, 0);
	if (una_clave->map == MAP_FAILED) {
		close(fd);
		perror("Error la mapear el archivo");
		exit(EXIT_FAILURE);
	}
	una_clave->fd = fd; //para despues cerrarlo si hace falta

	//quizas seria mejor un diccionario
	list_add(lista_claves, una_clave);
}

void procesarHeader (content_header* header, entrada_t* tabla){
	switch (header->id){
		case 11 : {
			// compactar

			sem_post(&semCompactar);
			
			break;
		}

		case 12 : { //SET			
			char *mensaje_recv = malloc (header->lenClave + header->lenValor);
			content* mensaje = (content*)malloc(sizeof(content));
    		mensaje->clave = malloc(header->lenClave + 1);
    		mensaje->valor = malloc(header->lenValor + 1);

			recv(coordinator_socket, mensaje_recv, header->lenClave + header->lenValor, 0);

			memcpy(mensaje->clave, mensaje_recv, header->lenClave);
			mensaje->clave[header->lenClave] = '\0';
			memcpy(mensaje->valor, mensaje_recv + header->lenClave, header->lenValor);
			mensaje->valor[header->lenValor] = '\0';

			log_warning(logger, "Key: %s, Value: %s", mensaje->clave, mensaje->valor);
			int posicion = consultarTabla (tabla, mensaje, header->lenValor);	

			// si no Hay Lugar
			if (noHayLugar){
				//hay que compactar

				//msjAlCoordinador = "compactar"
				send_header(coordinator_socket, 11);

				//activar semaforo 

				int posicion = consultarTabla (tabla, mensaje, strlen(mensaje->valor));

			}

			guardarEnTabla (tabla, mensaje, posicion);
			guardarEnMem (mensaje, posicion);

			//msjAlCoordinador = "SET_OK"
			send_header(coordinator_socket, 12);

			free(mensaje);
			break;
		}
		case 13:{ //GET
			//crear archivo con la clave que tenga la posicion en la tabla y el valor
			//ver mmap
			char * clave = (char*)malloc (header->lenClave + 1);

			recv(coordinator_socket, clave, header->lenClave + 1, 0);
			//deserializar
			clave[header->lenClave + 1] = '\0';
			//revisar si la clave ya existe
			if(!revisarLista(clave))
			{
				guardarEnClaves(header, clave);
				log_warning(logger, "Creating file for Key: %s", clave);
			}						
			
			//avisar al coordinador que creo el archivo (ID = 12)
			send_header(coordinator_socket, 12);
			break;
			/* A esta parte la dejo comentada por ahora porque no estoy seguro de donde habria que leberar los recursos, sepues lo acomodo

			if (munmap(map, content_header->lenValor) == -1) {
			perror("Error un-mmapping the file");

			}

   			 close(fd);
			*/

		}
	}

}

void send_header(int socket, int id)
{
    content_header* header = malloc(sizeof(content_header));
    header->id = id;
    header->lenClave = 0;
    header->lenValor = 0;
        
    send(socket, header, sizeof(content_header), 0);
}

map_t * buscar_por_clave(t_list* lista_claves, char* clave)
{
	bool _es_esta(map_t* p)
	{
		return string_equals_ignore_case(p->clave, clave);
	}

	return list_find(lista_claves, _es_esta);
}

void compactar (entrada_t * tabla){

	sem_wait(&semCompactar);

	int clavesVacias = 0;
	int j = 0;
	for (int i = 0; i <= configuracion->cantEntradas; i++){
		if (tabla[i].clave == "vacio"){
			int clavesVacias = 1;
			int j = i;
			while ((tabla[j].clave == "vacio") && (j <= configuracion->cantEntradas)){
				clavesVacias++;
				j++;
			}
			for (int j = i; j+clavesVacias ; j++){
				tabla[j].clave == tabla[j+clavesVacias].clave;
			}
		}	
	}

	//return NULL;
}