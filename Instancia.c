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
char* algReemplazo;
int dumpTimer;
int noHayLugar;
char* mem; //storage
pthread_t hiloDump;
int indexCircular = 0;
int comienzoDeEntradasLibres = 0;
int flagCompactar = 0;

int main(void)
{
	pthread_mutex_init(&mutex_dump, NULL);
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
		tabla[i].age = 0;
	}

	lista_claves = list_create();
	//ptrLista listaGETs = NULL; lista_claves es parecida

	mem = malloc(sizeof (char) *configuracion->cantEntradas*configuracion->tamanioEntradas);

	int error = pthread_create(&hiloDump, NULL, (void *)dump, tabla);
	if(error != 0)
	{
		log_error(logger, "Couldn't create thread Dump");
	}

	while(1)
	{
		pthread_mutex_lock(&mutex_dump);

		content_header *header = malloc (sizeof (content_header));

		recv(coordinator_socket, header, sizeof (content_header), 0);
		procesarHeader (header, tabla);

		free(header);

		pthread_mutex_unlock(&mutex_dump);
	}

	return EXIT_SUCCESS;
}

void configure_logger()
{
  logger = log_create("Instancia.log", "Instancia", false, LOG_LEVEL_INFO);
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
	get_string_value(logger, "AlgoritmoReemplazo", &algReemplazo, config);
	get_int_value(logger, "IntervaloDump", &dumpTimer, config);
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

  	getaddrinfo(ip, port, &hints, &server_info);  // Cambiar IP y PORT por los del archivo de config

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

	free(header_c);
}

void recibirTamanos ()
{
	configuracion = (configuracion_t*)malloc(sizeof(configuracion_t));

	recv(coordinator_socket, configuracion, sizeof(configuracion_t), 0);
	log_info(logger, "Received Number of Entries: %d", configuracion->cantEntradas);
	log_info(logger, "Received Size of Entries: %d", configuracion->tamanioEntradas);
}

int consultarTablaCIRC (entrada_t* tabla, content* mensaje){
// Se fija si la clave ya existe en la tabla devuelve esa posicion
	
	for (int i = 0; i <= comienzoDeEntradasLibres; i++){
		
		if (string_equals_ignore_case(tabla[i].clave, mensaje->clave)){
			return i;
		}
	}
	return -1; // Si la clave no esta en la tabla
}
				
int consultarTablaLRU (entrada_t* tabla, content* mensaje, int* laMasVieja){
// Va envejeciendo todo por el camino (de punta a punta). Si la clave existe devuelve su posicion
// Si la clave no existe eventualmente se puede pisar la clave con mayor edad (en otra funcion)
		
	for (int i = 0; i <= comienzoDeEntradasLibres; i++){
		
		*laMasVieja = 0;
		tabla[i].age++;
		if (*laMasVieja < tabla[i].age) {
			*laMasVieja = tabla[i].age;
		}
		
		if (string_equals_ignore_case(tabla[i].clave, mensaje->clave)){
			return i;
		}		
	}
	return -1; // Si la clave no esta en la tabla
}

int consultarTabla (entrada_t* tabla, char* clave){
// Se fija si la clave ya existe en la tabla devuelve esa posicion
	
	for (int i = 0; i < configuracion->cantEntradas; i++){
		
		if (string_equals_ignore_case(tabla[i].clave, clave)){
			return i;
		}
	}
	return -1; // Si la clave no esta en la tabla
}

void guardarEnMem (content* mensaje, int posicion){

	//////// GUARDADO EN MEMORIA
	int ubicacionEnMem = posicion * configuracion->tamanioEntradas;
	memcpy (mem + ubicacionEnMem , mensaje->valor , strlen(mensaje->valor) + 1);

	//////////HASTA ACA GUARDADO EN MEMORIA

	//aca hay que escribir el archivo	
	//buscar en la lista la claves	
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

	int cantPaginas = 0;
	int tamanioMensaje = header->lenValor;
	cantPaginas = getCantPaginas (tamanioMensaje);

	int result;
	int fd = open(clave, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) {
		perror("Error al intentar abrir el archivo");
		exit(EXIT_FAILURE);
	}

	result = lseek(fd, cantPaginas*configuracion->tamanioEntradas +1, SEEK_SET);
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
	una_clave->map = mmap(NULL, cantPaginas*configuracion->tamanioEntradas, PROT_WRITE, MAP_SHARED, fd, 0);
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
		case 11 : { // compactar			

			compactar(tabla);
			
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

			free(mensaje_recv);

			log_info(logger, "Key: %s, Value: %s", mensaje->clave, mensaje->valor);			
			
			//revisar si la clave ya existe
			if(!revisarLista(mensaje->clave))
			{
				guardarEnClaves(header, mensaje->clave);
				log_info(logger, "Creating file for Key: %s", mensaje->clave);
			}

			int cantPaginas = 0;
			int tamanioMensaje = strlen(mensaje->valor);
			
			if (string_equals_ignore_case(algReemplazo, "CIRC")){
				
				cantPaginas = getCantPaginas (tamanioMensaje);
				// recorro la tabla hasta el comienzoDeEntradasLibres
				int posicion = consultarTablaCIRC (tabla, mensaje);
				
				if (posicion != -1){ // Si encontro la clave en la tabla, libera las entradas si ahora ocupara menos
					int e = posicion + cantPaginas;
					tabla[posicion].tamanio = strlen(mensaje->valor);
					while (tabla[e].clave == mensaje->clave){
						strcpy(tabla[e].clave, "vacio");
						e++;
					}
					guardarEnMem (mensaje, posicion);
				} else { // Si la clave no esta en la tabla
					guardarEnTablaCIRC(tabla, mensaje, cantPaginas);
				}
			
			} else if (string_equals_ignore_case(algReemplazo, "LRU")){
			
				int laMasVieja = 0;
				cantPaginas = getCantPaginas (tamanioMensaje);
				// recorro la tabla de punta a punta envejeciendo todo y actualizo el valor de laMasVieja
				int posicion = consultarTablaLRU (tabla, mensaje, &laMasVieja);
				
				if (posicion != -1){ // Si encontro la clave en la tabla, libera las entradas si ahora ocupara menos
					int e = posicion + cantPaginas;
					tabla[posicion].tamanio = strlen(mensaje->valor);
					tabla[posicion].age = 0;
					while (tabla[e].clave == mensaje->clave){
						strcpy(tabla[e].clave, "vacio");
						tabla[e].age = 0;
						e++;
					}
					guardarEnMem (mensaje, posicion);
				} else { // Si la clave no esta en la tabla
					guardarEnTablaLRU(tabla, mensaje, &cantPaginas, &laMasVieja);
				}			
			}				
			
			//para que el coordinador funcione usando LSU
			int entradas_libres = free_entries(tabla);

			send_header_with_length(coordinator_socket, 12, entradas_libres, 0);

			free(mensaje->clave);
			free(mensaje->valor);
			free(mensaje);

			break;
		}
		case 13:{ //GET
			//No hace nada

			char * clave = (char*)malloc (header->lenClave + 1);

			recv(coordinator_socket, clave, header->lenClave + 1, 0);
			//deserializar
			clave[header->lenClave] = '\0';

			send_header(coordinator_socket, 12);

			free(clave);
			break;
		}
		case 14:{ // STORE
			char * clave = (char*)malloc (header->lenClave + 1);
			int cantEntradas = 0;

			recv(coordinator_socket, clave, header->lenClave + 1, 0);
			//deserializar
			clave[header->lenClave] = '\0';

			storeKey(tabla, clave);
		
			//avisar al coordinador que creo el archivo (ID = 12)
			send_header(coordinator_socket, 12);
			break;			
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

	free(header);
}

void send_header_with_length(int socket, int id, int len1, int len2)
{
    content_header* header = malloc(sizeof(content_header));
    header->id = id;
    header->lenClave = len1;
    header->lenValor = len2;
        
    send(socket, header, sizeof(content_header), 0);

	free(header);
}

int free_entries(entrada_t * tabla)
{
	int free_entries = 0;

	for(int i = 0; i < configuracion->cantEntradas; i++)
	{
		if(string_equals_ignore_case(tabla[i].clave, "vacio"))
			free_entries++;
	}

	return free_entries;
}

map_t * buscar_por_clave(t_list* lista_claves, char* clave)
{
	bool _es_esta(map_t* p){
		return string_equals_ignore_case(p->clave, clave);
	}

	return list_find(lista_claves, _es_esta);
}

void compactar (entrada_t * tabla){

	//Compactar tabla

	int clavesVacias = 0;
	int j = 0;
	for (int i = 0; i <= configuracion->cantEntradas; i++){
		if (tabla[i].clave == "vacio"){
			//int clavesVacias = 1;
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

	// Una vez que se compacto, se actualiza el comienzoDeEntradasLibres

	// HACER: actualizar el storage
	int e = 0;
	for (e = 0; tabla[e].clave!= "vacio" && e < configuracion->cantEntradas; e++){
		//log_info(logger, "%s", tabla[e].clave);	
	}
	comienzoDeEntradasLibres = e;
	indexCircular = e;
}

int getCantPaginas (int tamanioMensaje ){
	int cantPaginas = 0;
	
	if (tamanioMensaje % configuracion->tamanioEntradas == 0){
		cantPaginas = tamanioMensaje / configuracion->tamanioEntradas;
	} else{
		cantPaginas = (tamanioMensaje / configuracion->tamanioEntradas) + 1;
	}

	log_info(logger, "tamanio mensaje: %d", tamanioMensaje);
	log_info(logger, "cantidad de entradas: %d", cantPaginas);
	return cantPaginas;
}

void guardarEnTablaCIRC(entrada_t * tabla, content* mensaje, int cantPaginas){

	//POSIBLEMENTE HAY QUE REHACER
	//(indexCiruclar solo deberia usarse para reemplazar claves)
	/*Los pasos a seguir tanto para circ y lru son:
		1. encontrar un lugar vacio
		2a. no hay lugar, compactar
			3. encontrar otra vez un lugar vacio
			4. no hay lugar, reemplazar (usando algoritmo)

		2b. ubicar en ese lugar*/
	
	if (configuracion->cantEntradas - indexCircular >= cantPaginas){ 
	// Si hay lugares libres en la tabla (sin contar lo que sea propio de la fragmentacion).
		
		for (int i = indexCircular; i < (indexCircular + cantPaginas); i++){
			//guardar en tabla el tamaño del valor
			strcpy(tabla[i].clave , mensaje->clave);
			tabla[i].tamanio = strlen(mensaje->valor);
			tabla[i].age = 0;
			comienzoDeEntradasLibres++;
		}
						
		guardarEnMem(mensaje, indexCircular);
		indexCircular = indexCircular + cantPaginas;
		for(int x = 0; x < configuracion->cantEntradas; x++)
			log_info(logger, "%s", tabla[x].clave);
	} else {
	// Si no hay suficiente lugar libre: se compacta y el indexCircular vuelve al principio			
		switch (flagCompactar){
			case 0 : {
				log_info(logger, "Compactando");
				compactar(tabla);
				flagCompactar = 1;
				guardarEnTablaCIRC(tabla, mensaje, cantPaginas);
				break;
			}
			case 1 : {
				indexCircular = 0;
				flagCompactar = 0;
				guardarEnTablaCIRC(tabla, mensaje, cantPaginas);
				break;
			}
		}						
	}
}

void guardarEnTablaLRU(entrada_t * tabla, content* mensaje, int* cantPaginas,int* laMasVieja){
	
	if (configuracion->cantEntradas - comienzoDeEntradasLibres >= *cantPaginas){ 
	// Si hay lugares libres en la tabla (sin contar lo que sea propio de la fragmentacion).
		
		for (int i = comienzoDeEntradasLibres; i <= *cantPaginas; i++){
			//guardar en tabla el tamaño del valor
			strcpy(tabla[i].clave , mensaje->clave);
			tabla[i].tamanio = strlen(mensaje->valor);
			tabla[i].age = 0;			
			guardarEnMem(mensaje, comienzoDeEntradasLibres);
		}	
		comienzoDeEntradasLibres = comienzoDeEntradasLibres + *cantPaginas;
	} else {
	// Si no hay suficiente lugar libre: Se libera la entrada LRU y si fuera necesario, se compacta.
		for (int i = 0; i <= comienzoDeEntradasLibres; i++){
			int auxLaMasVieja = 0;
			*laMasVieja = 0;
			if (auxLaMasVieja < tabla[i].age && tabla[i].age < *laMasVieja) {
				auxLaMasVieja = tabla[i].age;
			}
			if (tabla[i].age == *laMasVieja){
				strcpy(tabla[i].clave, "vacio");
				tabla[i].age = 0;
			}
			*laMasVieja = auxLaMasVieja;
			compactar(tabla);
			guardarEnTablaLRU(tabla, mensaje, cantPaginas, laMasVieja);
		}				
	}		
}

void storeKey(entrada_t * tabla, char* clave){
	int posicion = consultarTabla (tabla, clave);
	if (posicion == -1){ // Si quiero hacer STORE de una key que no está más en la tabla
		// send error to coordinator
		send_header(coordinator_socket, 15);		
		log_info(logger, "ERROR: La clave %s ha sido reemplazada", clave);
	}else{ // Si está en la tabla		
		int ubicacionEnMem = posicion * configuracion->tamanioEntradas;

		//// STORE
		map_t* una_clave = buscar_por_clave(lista_claves, clave);
		if(una_clave == NULL)
			log_error(logger, "ERROR");

		memcpy(una_clave->map, mem + ubicacionEnMem, tabla[posicion].tamanio +1 );
	}									
}

void dump(entrada_t * tabla)
{
	while(true)
	{
		//dormir por cantidad de tiempo(en segundos) de configuracion
		sleep(dumpTimer);

		//semaforo mutex para evitar condicion de carrera
		pthread_mutex_lock(&mutex_dump);

		log_info(logger, "Starting Dump");

		//por cada clave de la tabla hacer store
		for(int i = 0; i < configuracion->cantEntradas; i++)
		{
			if(string_equals_ignore_case(tabla[i].clave, "vacio"))
				continue;
			
			int ubicacionEnMem = i * configuracion->tamanioEntradas;

			map_t* una_clave = buscar_por_clave(lista_claves, tabla[i].clave);
			if(una_clave == NULL)
			{
				exit_with_error(logger, "Key is not on the list, failed to dump");
			}

			memcpy(una_clave->map, mem + ubicacionEnMem, tabla[i].tamanio +1 );

			int cant_paginas = getCantPaginas(tabla[i].tamanio);

			//salteo las entradas de la misma clave
			//(menos uno porque el for me lo incrementa tambien)
			i += cant_paginas - 1;
		}

		pthread_mutex_unlock(&mutex_dump);

		log_info(logger, "Finished Dump");
	}	
}