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
pthread_mutex_t lock;
int error;
fd_set read_fds;
int master_socket, new_socket , client_socket[MAX_CLIENTS] , 
    max_clients = MAX_CLIENTS , i;
int addrlen = sizeof(serverAddress);
char * message = "Welcome";
content_header * header_c;

int main(void) 
{
	pthread_mutex_init(&lock, NULL);
    configure_logger();

    config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");

    get_config_values(config);   

    instances = list_create();

    create_server();
    host_connections();

	return EXIT_SUCCESS;
}

void get_config_values(t_config* config)
{
    if(config_has_property(config, "PuertoEscucha"))
        {
            listeningPort = config_get_int_value(config, "PuertoEscucha");
            log_info(logger, "Port from config file: %d", listeningPort);
        }
    else
        exit_with_error(logger, "Cannot read port from config file");

    if(config_has_property(config, "AlgoritmoDistribucion"))
        {
            char* alg_aux = config_get_string_value(config, "AlgoritmoDistribucion");
            algorithm = to_algorithm(alg_aux);
            log_info(logger, "Algorithm from config file: %s", alg_aux);
        }
    else
        exit_with_error(logger, "Cannot read algorithm from config file");
}

void create_server()
{  
	char * buffer;

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
}

void host_connections()
{	 
    int valread;
    
    while(1)
	{
        //pthread_mutex_lock(&lock);
        FD_ZERO(&read_fds);
        FD_SET(master_socket, &read_fds);

        if((new_socket = accept(master_socket, (struct sockaddr*)&serverAddress, &addrlen)) <= 0)
        {
            perror("Accept: ");
        }

        log_info(logger, "New connection , socket fd is %d , ip is : %s , port : %d", 
				new_socket , inet_ntoa(serverAddress.sin_addr) , ntohs(serverAddress.sin_port));   

        pthread_mutex_lock(&lock);
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            if(!FD_ISSET(client_socket[i], &read_fds))
            {
                client_socket[i] = new_socket;
                i = max_clients;
            }
        }
        pthread_mutex_unlock(&lock);

        FD_SET(new_socket, &read_fds);

        pthread_t id_client;
        
        if(send(new_socket, message, strlen(message), 0) <= 0)  
        {  
        perror("send"); 
        }  
        
        log_info(logger, "Welcome message sent successfully");

        header_c = (content_header*) malloc(sizeof(content_header));
        //Check if it was for closing , and also read the 
        //incoming message 
        if ((valread = recv(new_socket ,header_c, sizeof(content_header), 0)) == 0)
        {
            log_info(logger, "New socket disconnected");
            continue;                     
        }

        process_message_header(header_c, new_socket);
        
        thread_data_t thread_data;

        thread_data.next_message_len = header_c->len;
        thread_data.socket = new_socket;

        switch(hello_id)
        {
            case INSTANCE:
            {
                if(pthread_create(&id_client, NULL, (void*)host_instance, (void*)&thread_data) != 0 )
                    exit_with_error(logger, "Failed to create thread");
                break;
            }
            case ESI:
            {
                if(pthread_create(&id_client, NULL, (void*)host_esi, (void*)&thread_data) != 0 )
                    exit_with_error(logger, "Failed to create thread");
                break;
            }
            case SCHEDULER:
            {
                if(pthread_create(&id_client, NULL, (void*)host_scheduler, (void*)&thread_data) != 0 )
                    exit_with_error(logger, "Failed to create thread");
                break;
            }
            default:
            {
                log_error(logger, "Incorrect Hello ID");
            }
        }    
	}
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

void host_instance(void* arg)
{
    thread_data_t *data = (thread_data_t *) arg;
    int socket = data->socket;
    int len = data->next_message_len;

    log_info(logger,"Connected with an Instance");

    int valread;
    char* name = malloc(len + 1);
    
    //recibe el nombre de la instancia
    int result = recv(socket, name, len, 0);
    if(result < 0)
        exit_with_error(logger,"Cannot recieve message");
    if(result == 0)
        disconnect_socket(socket, true);

    instance_t * instance;
    instance = malloc(sizeof(instance_t));
    instance->socket = socket;
    instance->is_active = 1;

    add_instance_to_list(name, socket, instance);

    char* header = malloc(sizeof(content_header));

    while(1)
    {
        pthread_mutex_lock(&instance->start);

    //enviar header con tamaño de clave y valor
    //enviar clave y valor
        log_warning(logger, "DEBUG: i'm the chosen one");
        //resultado de la operacion
        /*if ((valread = recv(socket ,header, sizeof(content_header), 0)) == 0)
            disconnect_socket(socket, true);*/

        pthread_mutex_lock(&instance->start);
    }

    free(header);
}

void host_esi(void* arg)
{
    thread_data_t *data = (thread_data_t *) arg;
    int socket = data->socket;
    int len = data->next_message_len;

    int valread;
    content_header* header = malloc(sizeof(content_header));

    while(1)
    {
        if ((valread = recv(socket ,header, sizeof(content_header), 0)) == 0)
            disconnect_socket(socket, false);
        process_message_header(header, socket);
    }

    free(header);
}

void host_scheduler(void* arg)
{
    thread_data_t *data = (thread_data_t *) arg;
    int socket = data->socket;
    int len = data->next_message_len;

    int valread;
    char* header = malloc(sizeof(content_header));

    while(1)
    {
        if ((valread = recv(socket ,header, sizeof(content_header), 0)) == 0)
            disconnect_socket(socket, false);
    }

    free(header);
}

void disconnect_socket(int socket, bool is_instance)
{
    //disconnected , get his details and print 
    pthread_mutex_lock(&lock);
    if(is_instance)
        disconnect_instance_in_list(socket);
    getpeername(socket , (struct sockaddr*)&serverAddress , \
        (socklen_t*)&addrlen);  
    log_info(logger, "Host disconnected , ip %s , port %d", 
        inet_ntoa(serverAddress.sin_addr) , ntohs(serverAddress.sin_port));  
        
    //Close the socket and mark as 0 in list for reuse 
    close_socket(socket);
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

void close_socket(int socket)
{
    close(socket); 
    for(int j = 0; j < max_clients; j++)
    {
        if(client_socket[j] == socket)
            client_socket[j] = 0;
    }   
}

void process_message_header(content_header* header, int socket)
{
    switch(header->id)
    {
        case 10:
        {
            hello_id = INSTANCE;
            break;
        }
        case 20:
        {
            hello_id = ESI;
            log_info(logger,"Connected with an ESI");
            //agregar a vector de esis
            break;
        }
        case 22: //ESI pide un SET
        {
            log_info(logger, "ESI requested a SET");
            //posible semaforo
            assign_instance(algorithm, instances);
            break;
        }
        case 30:
        {
            hello_id = SCHEDULER;
            log_info(logger,"Connected with the Scheduler");
            //agregar a una variable
            break;
        }
        default:
        {
            log_error(logger, "Incorrect ID message");
            break;
        }
    }
}

void assign_instance(_Algorithm algorithm, t_list* instances)
{
    switch(algorithm)
    {
        case EL:
        {
            instance_t* chosen_one = find_by_times_used(instances);
            pthread_mutex_unlock(&chosen_one->start);
            break;
        }
    }
}

void add_instance_to_list(char* name, int socket, instance_t* inst_aux)
{
    log_info(logger, "Received Instance name: %s", name);

    //agregar a diccionario de instancias
    inst_aux = name_is_equal(instances, name);
    if(inst_aux != NULL)
    { //si ya esta en la lista
        pthread_mutex_lock(&inst_aux->start);
        list_add(instances, inst_aux);
        log_info(logger, "Updated state of: %s", name);
    }
    else
    { //si no esta en la lista
        inst_aux->name = name;
        inst_aux->times_used = 0;
        inst_aux->space_used = 0;
        pthread_mutex_init(&inst_aux->start, NULL);
        pthread_mutex_lock(&inst_aux->start);
        list_add(instances, inst_aux);
        log_info(logger, "New Instance added to list");
    }
}

void disconnect_instance_in_list(int socket)
{
    instance_t* instance = socket_is_equal(instances, socket);
    if(instance != NULL)
    {
        instance->is_active = 0;
        pthread_mutex_unlock(&instance->start);
        list_add(instances, instance);
        log_info(logger, "Removed from list of active Instances");
    }
    else
    {
        log_warning(logger, "Tried to delete an instance not added before");
    }
}

instance_t* name_is_equal(t_list* lista, char* name)
{
    bool _is_the_one(instance_t* p) 
    {
        return string_equals_ignore_case(p->name, name);
    }
    return list_remove_by_condition(lista, _is_the_one);
}

instance_t* socket_is_equal(t_list* lista, int socket)
{
    bool _is_the_one(instance_t* p) 
    {
        return p->socket == socket;
    }
    return list_remove_by_condition(lista, _is_the_one);
}

instance_t* find_by_times_used(t_list* lista)
{
    bool _is_active(instance_t* p) 
    {
        if(p->is_active == 1)
            return true;
        else
            return false;
    }
    bool _lower_than_the_next(instance_t* p, instance_t* q) 
    {
        if(p->times_used < q->times_used)
            return true;
        else
            return false;
    }
    t_list* list_aux;

    list_aux = list_filter(lista, _is_active);
    list_sort(list_aux, _lower_than_the_next);

    return list_take(lista, 1);
}

_Algorithm to_algorithm(char* string)
{
    if(string_equals_ignore_case(string, "EL"))
        return EL;
    if(string_equals_ignore_case(string, "LSU"))
        return LRU;
    if(string_equals_ignore_case(string, "KE"))
        return KE;
    log_warning(logger, "Incorrect algorithm type, using default EL");
    return EL;
}