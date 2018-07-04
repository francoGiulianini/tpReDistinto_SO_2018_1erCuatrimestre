/*
 ============================================================================
 Name        : Planificador.c
 Author      : 
 Version     :
 Copyright   : Los Mas Mejores ©
 Description : Planificar Scripts en C, Ansi-style
 ============================================================================
 */
#include "Planificador.h"
#include "Consola.h"
//#include <ConfigGetValues.h>

pthread_t idConsole;
pthread_t idHostConnections;
struct sockaddr_in serverAddress;
int fin_de_esi = 0;
int respuesta_ok = 1;
int key_blocked = 0;
int abort_esi = 0;
/* int kick_esi = 0; #DUDA	No creo que sea necesario este flag, xq creo que hasta complica mas que facilitar las cosas,
							sobre todo debido al chequeo en el while del main. Es mejor que todo ese comportamiento lo tenga unlock_key */

int main(void)
{
	configure_logger();
    sem_init(&hay_esis, 0, 0);
    sem_init(&esi_executing, 0, 0);
    sem_init(&coordinador_pregunta, 0, 0);
    sem_init(&esi_respuesta, 0, 0);
    pthread_mutex_init(&status_mutex, NULL);
    pthread_mutex_init(&pause_mutex, NULL);
    pthread_mutex_init(&new_esi, NULL);
    pthread_mutex_init(&cola_bloqueados_mutex, NULL);
    lista_ready = list_create();
    lista_bloqueados = list_create();
    claves_bloqueadas_por_esis = list_create();
    finished_esis = queue_create();

    config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");

    get_values_from_config(logger, config);

    new_blocked_keys(); //agrega las claves bloqueadas desde config
	
    //aqui se conecta con el coordinador
    socket_c = connect_to_server(ip_c, port_c, "Coordinator");
    send_header(socket_c, 30);

	int error = pthread_create(&idConsole, NULL, (void*)Console, NULL);
	if(error != 0)
	{
		log_error(logger, "Couldn't create Console");
	}

	//aca se conecta con los esis
	error = pthread_create(&idHostConnections, NULL, (void*)HostConnections, NULL); 
	if(error != 0)
	{
		log_error(logger, "Couldn't create Server Thread");
	}

    sem_wait(&hay_esis);
    wait_start(socket_c); //espera que le manden un id 35

    t_esi * un_esi;

    pthread_mutex_lock(&new_esi);
    un_esi = list_remove(lista_ready, 0);
    pthread_mutex_unlock(&new_esi);
    log_warning(logger, "%s Chosen to Start", un_esi->name);
	//Codigo del Planificador
	while (stop != 1)
	{    
        pthread_mutex_lock(&pause_mutex);
        pthread_mutex_lock(&status_mutex);
        
        if(un_esi == NULL)
        {
            exit_with_error(logger, "Cannot find ESI");
        }
        send_header(un_esi->socket, 21); //ejecutar una instruccion

        wait_question(socket_c);

        sem_wait(&esi_respuesta);

        update_values();

        //#TODO:

        /*if(blocked_esi_by_console)
        {
            blocked_esi_by_console = 0;
            sem_post(&esi_executing);
            pthread_mutex_unlock(&pause_mutex);
            continue;
        }*/ 
        //LA CLAVE SE BLOQUEA NO EL ESI

        if(!respuesta_ok)
        {
            if(abort_esi) //enviar a cola finalizados
                finish_esi(un_esi);
            else //ya se bloqueo en la consulta del coordinador
                fin_de_esi = 1;
        }

        pthread_mutex_unlock(&status_mutex);
        pthread_mutex_unlock(&pause_mutex);

        if(fin_de_esi /*|| kick_esi*/)
        {
           /*if(kick_esi)	//#DUDA siempre que se hace kick_esi, debe entrar otro ESI, asi que en realidad mas que send_esi_to_ready, deberia tambien mandar otro_esi a ejecucion
                send_esi_to_ready(un_esi);
            else
                */finish_esi(un_esi);//pregunta si se bloqueo o finalizo o recien empieza
            sem_wait(&hay_esis);
            un_esi = list_remove(lista_ready, 0);
            fin_de_esi = 0;
        }
	}

    pthread_join(idConsole, NULL);
    pthread_mutex_destroy(&pause_mutex);
    log_destroy(logger);
	return EXIT_SUCCESS;
}

void exit_with_error(t_log* logger, char* error_message)
{
    log_error(logger, error_message);
    log_destroy(logger);
    exit(EXIT_FAILURE);
}

void get_values_from_config(t_log* logger, t_config* config)
{
    get_int_value(logger, "PuertoEscucha", &listeningPort, config);
    get_string_value(logger, "PuertoCoordinador", &port_c, config);
    get_string_value(logger, "IPCoordinador", &ip_c, config);
    char* algorithm_aux;
    get_string_value(logger, "AlgoritmoPlanificacion", &algorithm_aux, config);
    algorithm = to_algorithm(algorithm_aux);
    get_int_value(logger, "EstimacionInicial", &initial_estimation, config);
    get_int_value(logger, "AlfaPlanificacion", &alpha, config);
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

void configure_logger()
{
    logger = log_create("Planificador.log", "Planificador", true, LOG_LEVEL_INFO);
}

void new_blocked_keys()		//#DUDA Me hace mucho ruido el funcionamiento de esto, y los malloc con el for, cuando se liberan?
{
    char** blocked_keys = config_get_array_value(config, "ClavesBloqueadas");

    for(int i = 0; blocked_keys[i] != NULL; i++)
    {
        clave_bloqueada_t * clave_bloqueada = malloc(sizeof(clave_bloqueada_t));

        clave_bloqueada->cola_esis_bloqueados = queue_create();
        t_esi* one_esi = malloc(sizeof(t_esi));
        one_esi->name = "CONSOLA";
        queue_push(clave_bloqueada->cola_esis_bloqueados, one_esi);
        clave_bloqueada->key = blocked_keys[i];

        list_add(lista_bloqueados, clave_bloqueada);
        log_info(logger, "Updated Blocked Keys with %s", clave_bloqueada->key);
    }
}

void HostConnections()
{
	char * message = "Welcome";
    int num_esi = 0;
	int opt = 1;
	int master_socket, new_socket , client_socket[MAX_CLIENTS] , 
        max_clients = MAX_CLIENTS , activity, i , valread , sd;  
    int max_sd;  
	int addrlen = sizeof(serverAddress);
	char * buffer;
	fd_set read_fds;

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(listeningPort);

	for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }

    //Socket for listening
	master_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (master_socket < 0)
		exit_with_error(logger, "Failed to create Socket");

	int activated = 1;

	setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &activated, sizeof(activated));

	if (bind(master_socket, (void *)&serverAddress, sizeof(serverAddress)) != 0)
		exit_with_error(logger, "Failed to bind");

	log_info(logger, "Waiting for connections...");
	listen(master_socket, 100);

	while(1)
	{
		FD_ZERO(&read_fds);

		FD_SET(master_socket, &read_fds);  
        max_sd = master_socket;

		//add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
               
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &read_fds);  
               
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }

		activity = select( max_sd + 1 , &read_fds , NULL , NULL , NULL);

		if ((activity < 0) && (errno!=EINTR))  
        {  
            log_error(logger, "Select error");  
        }

		//If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &read_fds))  
        {  
            if ((new_socket = accept(master_socket, (struct sockaddr*)&serverAddress, &addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
           
            //inform user of socket number - used in send and receive commands 
            log_info(logger,
				"New connection , socket fd is %d , ip is : %s , port : %d", 
				new_socket , inet_ntoa(serverAddress.sin_addr) , ntohs(serverAddress.sin_port));  
         
            //send new connection greeting message 
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
            {  
                perror("send");  
            }  
               
            log_info(logger, "Welcome message sent successfully");  
               
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    log_info(logger, "Adding to list of sockets as %d" , i);  
                    
                    break;  
                }  
            }  
        }

		//else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
               
            if (FD_ISSET( sd , &read_fds))  
            {  
                content_header * header = (content_header*) malloc(sizeof(content_header));

                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = recv(sd ,header, sizeof(content_header), 0)) == 0)  //TIENE QUE SEGUIR EL PROTOCOLO
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&serverAddress , \
                        (socklen_t*)&addrlen);  
                    log_info(logger, "Host disconnected , ip %s , port %d", 
                        inet_ntoa(serverAddress.sin_addr) , ntohs(serverAddress.sin_port));  
                       
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                   
                //check the message that came in 
                else 
                {  
                    //nuevo esi
                    if(header->id == 20)
                    {
                        //creamos la estructura del esi
                        pthread_mutex_lock(&new_esi);
                        t_esi * a_new_esi = malloc(sizeof(t_esi));
                        num_esi++;

                        char* name = (char*)malloc(7);
                        if(num_esi < 10)
                            sprintf(name, "ESI0%d", num_esi);
                        else 
                            sprintf(name, "ESI%d", num_esi);

                        a_new_esi->socket = sd;
                        a_new_esi->name = name;
                        a_new_esi->instructions_counter = 0;
                        a_new_esi->cpu_time_estimated = (float)initial_estimation;
                    
                        calculate_estimation(a_new_esi);

                        //se agrega a la lista de ready
                        list_add(lista_ready, a_new_esi);
                        pthread_mutex_unlock(&new_esi);                       
                        log_info(logger, "New ESI added to the queue as %s", a_new_esi->name);
                        log_info(logger, "Estimation: %f", a_new_esi->cpu_time_estimated);
                        sem_post(&hay_esis);

                        free(name);
                        free(a_new_esi);
                    }

                    if(header->id == 22)
                    {
                        respuesta_ok = 1;
                        sem_post(&esi_respuesta);
                    }
                    
                    if(header->id == 23)
                    {
                        respuesta_ok = 0;
                        sem_post(&esi_respuesta);
                    }

                    if(header->id == 24) //esi termino ejecucion
                    {
                        fin_de_esi = 1;
                        respuesta_ok = 1;
                        abort_esi = 1;
                        sem_post(&esi_respuesta);
                    }
                }

            	free(header);

            }  
        }
	}
}

int connect_to_server(char * ip, char * port, char *server)
{
	struct addrinfo hints;
  	struct addrinfo *server_info;

  	memset(&hints, 0, sizeof(hints));
  	hints.ai_family = AF_UNSPEC; 
  	hints.ai_socktype = SOCK_STREAM; 

  	getaddrinfo(ip, port, &hints, &server_info);

  	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (server_socket <= 0)
    {
        perror("client: socket");
        exit_with_error(logger, "Failed to create Socket");
    }
  	int res = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

  	freeaddrinfo(server_info);

  	if (res < 0) 
  	{
   		log_error(logger, "Cannot connect to: %s", server);
		exit_with_error(logger, "");
  	}
  
  	log_info(logger, "Connected with the: %s", server);

	char * buffer = (char*) calloc(sizeof(char), WELCOME_MSG);

	recv(server_socket, buffer, WELCOME_MSG, 0);

	log_info(logger, "%s", buffer);
	free(buffer);

	return server_socket;
}

void send_header(int socket, int id)
{
    content_header * header = malloc(sizeof(content_header));
    header->id = id;
    header->len = 0;
    header->len2 = 0;
    
    send(socket, header, sizeof(content_header), 0);
    free(header);
}

void wait_question(int socket)
{
    content_header* header = malloc(sizeof(content_header));

    recv(socket, header, sizeof(content_header), 0);
    log_info(logger, "Received header id: %d", header->id);

    switch (header->id)
    {
    	case 31:        //coordinador pregunta por clave bloqueada
        {	
            char* message = (char*)malloc(header->len + 1);

        	recv(socket, message, header->len, 0);
        	message[header->len] = '\0';

        	log_info(logger, "Coordinator asked to check Key: %s", message);
        	check_key(message);
        	free(message);
            break;
        }
    	case 32:        //coordinador pide desbloquear clave
        {
        	char* message = (char*)malloc(header->len + 1);

        	recv(socket, message, header->len, 0);
        	message[header->len] = '\0';

        	log_info(logger, "Coordinator asked to store Key: %s", message);
        	unlock_key(message);
        	free(message);
            break;
        }
    	case 33:        //coordinador no pregunta nada (operacion SET)
        	log_info(logger, "Coordinator asked to set a key");
            break;

    	case 34:
        	abort_esi = 1;
            break;

    	case 35:
        	log_info(logger, "All Connected, Initiating Scheduler");
            break;

    	default:
        	log_error(logger, "Message id not valid: %d", header->id);
        	exit_with_error(logger, "");
    }

    free(header);
}

void wait_start(int socket)
{
    content_header* header = malloc(sizeof(content_header));

    recv(socket, header, sizeof(content_header), 0);

    if (header->id = 35)
    {
        log_info(logger, "All Connected, Initiating Scheduler");
    }

    free(header);
}

/*
Funcion: check_key(char* key)
Descripcion: Verifica si la clave esta bloqueada o no
Comentarios:
    1- Busca la clave en la lista de claves bloqueadas
      1-1 si no existe, la crea
      1-2 si existe, se fija si la cola de esis esperando esa clave esta vacia
        1-2-1 si esta vacia, fijarse si un esi tiene la clave
        1-2-2 si el primer elemento es "CONSOLA" se bloquea
        //**1-2-3 si hay esis esperando, se bloquea <-- MAL**
*/
void check_key(char * key)
{
    clave_bloqueada_t* a_key = find_by_key(lista_bloqueados, key);
    int key_len = strlen(key) + 1;
    int id_len = strlen(un_esi->name);

	if(a_key == NULL) //si no encuentra la clave en la lista, la crea
	{
		log_warning(logger, "Requested Key doesnt exist, Adding Key to list");
        
        a_key->key = key;
        a_key->cola_esis_bloqueados = queue_create();

        list_add(lista_bloqueados, a_key);

        clave_bloqueada_por_esi_t* nueva_clave = malloc(sizeof(clave_bloqueada_por_esi_t));
        nueva_clave->esi_id = un_esi->name;
        nueva_clave->key = key;

        list_add(claves_bloqueadas_por_esis, nueva_clave);

        send_header(socket_c, 31); //31 clave libre
        free(nueva_clave);
	}
    else
    {
        if(queue_is_empty(a_key->cola_esis_bloqueados)) //la lista esta vacia
        {
            //#TODO: fijarse si un esi tiene la clave
            // usando lista claves_bloqueadas_por_esis

            clave_bloqueada_por_esi_t* nueva_clave = malloc(sizeof(clave_bloqueada_por_esi_t));
            nueva_clave->esi_id = un_esi->name;
            nueva_clave->key = key;

            list_add(claves_bloqueadas_por_esis, nueva_clave);
            /* REPETICION DE CODIGO /\ */

            send_header(socket_c, 31); //31 clave libre
            free(nueva_clave);
            return;
        }

        t_esi* one_esi = queue_peek(a_key->cola_esis_bloqueados);
        if(one_esi != NULL && string_equals_ignore_case(one_esi->name, "CONSOLA"))
        {
            //el primer elemento es CONSOLA
            //esta bloqueado por clave desde configuracion
            one_esi = queue_pop(a_key->cola_esis_bloqueados);
        }

        //la lista tiene elementos pero pudo haber sido desbloqueada por consola
        //#TODO: asegurarse de que todos los malloc tengan free
        //#TODO: fijarse si un esi tiene la clave
        // usando lista claves_bloqueadas_por_esis 

        log_warning(logger, "Requested Key was taken before, Adding ESI to blocked queue");

        send_header(socket_c, 32); //32 clave bloqueada
        //block_esi(un_esi, a_key);
        queue_push(a_key->cola_esis_bloqueados, un_esi);
    }
}

/*
Funcion: unlock_key(char* key)
Descripcion: Desbloquea a un esi de la cola de bloqueados
Comentarios: 
*/
void unlock_key(char* key)
{
    clave_bloqueada_t* a_key = find_by_key(lista_bloqueados, key);

    if(queue_is_empty(a_key->cola_esis_bloqueados))
        return;
    t_esi* otro_esi = queue_pop(a_key->cola_esis_bloqueados);
    
    switch(algorithm)
    {
        case SJFSD:
        {
            //estimar la rafaga del esi que se libera
            calculate_estimation(otro_esi);

            log_info(logger, "New Estimation for: %s is: %f",
                otro_esi->name, otro_esi->cpu_time_estimated);

            list_add(lista_ready, otro_esi);
            
            //reordenar la lista de ready
            
            break;
        }
        case SJFCD: //Precondicion: Asumo que no existe ESI en la cola de ready con rafaga mas corta que un_esi, por lo tanto:
        //#TODO: El SJFCD debería:

        /*
        
        1. calcular estimacion del otro_esi
        2. Chequear si otro_esi->cpu_time_estimated < un_esi->cpu_time_estimated,
            2.1 Si es cierto:
                2.1.1 Mandar un_esi a lista_ready,
                2.1.2 Meter a otro_esi en ejecución, #TODO #DUDA: Como se asigna un ESI a ejecucion?
            2.2 Si es falso:
            	2.2.1 Mandar otro_esi a lista_ready,

        */
        {
            calculate_estimation(otro_esi);		//revisar estimaciones de un_esi y el que se libera

            log_info(logger, "Estimation for: %s is: %f",
                otro_esi->name, otro_esi->cpu_time_estimated);


            if (otro_esi->cpu_time_estimated < un_esi->cpu_time_estimated)		//si el que se libera es mas chico desalojar = 1
            {
                //kick_esi = 1;
                //#DUDA: si yo activo el flag kick_esi pero dsps digo que un_esi = otro_esi, al retornar me terminaria pateando el ESI que TIENE que ejecutar
            	list_add(lista_ready, un_esi);
                un_esi = otro_esi; //un_esi siempre es el ESI que se está ejecutando
            }
            else
            {
            	list_add(lista_ready, otro_esi);
            }

            

            break;
        }
        case HRRN:
        {
            //no hay desalojo pero el esi desbloqueado tiene espera de 0;
            calculate_estimation(otro_esi);
            log_info(logger, "Estimation for: %s is: %f",
                otro_esi->name, otro_esi->cpu_time_estimated);

            otro_esi->waiting_time = 0;
            list_add(lista_ready, otro_esi);
            
            break;
        }
    }
    sort_list_by_algorithm(lista_ready);		//Independientemente de los casos, reordeno lista_ready
    sem_post(&hay_esis);
}

void update_values()
{
    if(respuesta_ok)
        un_esi->instructions_counter += 1;

    switch(algorithm)
    {
        case SJFSD:
        {
            un_esi->cpu_time_estimated -= 1;
            break;
        }
        case SJFCD:
        {
            un_esi->cpu_time_estimated -= 1;
            break;
        }
        case HRRN:
        {
            //Se agrega tiempo de espera a los ESI de lista_ready
            refresh_waiting_time(lista_ready);
            break;
        }
    }
}

void calculate_estimation(t_esi* esi_a_estimar)
{
    int i = esi_a_estimar->instructions_counter;
    int cpu = esi_a_estimar->cpu_time_estimated;

    float alpha_a = (((float)alpha / 100) * i);
    float alpha_b = (1 - ((float)alpha / 100)) * cpu;

    float nueva_estimacion = (alpha_a + alpha_b);

    esi_a_estimar->cpu_time_estimated = nueva_estimacion;
}

void sort_list_by_algorithm(t_list * list)
{
    bool _best_by_algorithm(t_esi* p, t_esi* q) 
    {
        switch(algorithm){
            case SJFSD:
            case SJFCD:
                if(p->cpu_time_estimated < q->cpu_time_estimated)
                  return true;
                else
                 return false;
            case HRRN:
                if(response_ratio(p) > response_ratio(q))
                  return true;
                else
                 return false;
        }
    }

    list_sort(list, _best_by_algorithm);
}

float response_ratio(t_esi * p)
{
    return (p->waiting_time + p->cpu_time_estimated)/p->cpu_time_estimated;
}

//#TODO:

/*void block_esi(t_esi * un_esi, clave_bloqueada_t* a_key)
{
    queue_push(a_key->cola_esis_bloqueados, un_esi);
}*/

void send_esi_to_ready(t_esi * un_esi)
{
    //agregar a lista ready
    list_add(lista_ready, un_esi);
    //ordenar lista acorde al algoritmo establecido por consola
    sort_list_by_algorithm(lista_ready);
    //kick_esi = 0;
}

void finish_esi(t_esi * un_esi)
{
    //agregar a cola finalizados
    queue_push(finished_esis, un_esi);
    //#TODO: liberar todos los recursos tomados por el ESI

    abort_esi = 0;
}

_Algorithm to_algorithm(char* string)
{
    if(string_equals_ignore_case(string, "SJF-SD"))
        return SJFSD;
    if(string_equals_ignore_case(string, "SJF-CD"))
        return SJFCD;
    if(string_equals_ignore_case(string, "HRRN"))
        return HRRN;
    log_warning(logger, "Incorrect algorithm type, using default SJF-SD");
    return SJFSD;
}

void refresh_waiting_time (t_list * list)
{
    /*t_link_element * list_node = list->head;

    for(list_node != NULL)
    {
        list_node->data->waiting_time += 1;
        list_node = list_node->next;
    }*/
    void _increase_time(t_esi* p)
    {
        p->waiting_time += 1;
    }

    list_iterate(lista_ready, _increase_time);
}