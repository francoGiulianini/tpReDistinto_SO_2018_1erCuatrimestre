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
int operation;
fd_set read_fds;
int master_socket, new_socket;
int addrlen = sizeof(serverAddress);
char * welcome_message = "Welcome";
content_header * header_c;
int key_is_not_blocked = 1;

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

    if(config_has_property(config, "Retardo"))
        {
            delay = config_get_int_value(config, "Retardo");
            log_info(logger, "Delay from config file: %d", delay);
        }
    else
        exit_with_error(logger, "Cannot read Delay from config file");
}

void create_server()
{
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(listeningPort);

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
        FD_ZERO(&read_fds);
        FD_SET(master_socket, &read_fds);

        if((new_socket = accept(master_socket, (struct sockaddr*)&serverAddress, &addrlen)) <= 0)
        {
            perror("Accept: ");
        }

        log_info(logger, "New connection , socket fd is %d , ip is : %s , port : %d", 
				new_socket , inet_ntoa(serverAddress.sin_addr) , ntohs(serverAddress.sin_port));   

        FD_SET(new_socket, &read_fds);

        pthread_t id_client;
        
        if(send(new_socket, welcome_message, strlen(welcome_message), 0) <= 0)  
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

    instance = add_instance_to_list(name, socket);

    char* header = malloc(sizeof(content_header));

    while(1)
    {
        sem_wait(&instance->start);

        //la instancia tiene que hacer algo previo?
        /*if ((valread = recv(socket ,header, sizeof(content_header), 0)) == 0)
            disconnect_socket(socket, true);*/

        log_warning(logger, "DEBUG: i'm the chosen one");
        //enviar header con tamaño de clave y valor
        //send(socket, header_nuevo, sizeof(header_nuevo), 0);

        //enviar clave y valor
        //send(socket, message_content, len, 0);
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
    sem_init(&esi_operation, 1, 0);
    sem_init(&scheduler_response, 1, 0);
    thread_data_t *data = (thread_data_t *) arg;
    int socket = data->socket;
    int len = data->next_message_len;
    int valread;

    while(1)
    {
        content_header* header = malloc(sizeof(content_header));
        int key_id;

        sem_wait(&esi_operation);
        header->len = strlen(message->key);
        switch(operation)
        {
            case 31: //preguntar por clave
            {
                header->id = 31;
                send(socket, header, sizeof(content_header*), 0);

                recv(socket, key_id, sizeof(int), 0);

                process_message_header(header, socket);
                sem_post(&scheduler_response);
                break;
            }
            case 32: //desbloquear clave
            {
                break;
            }
            case 33: //abortar esi
            {
                break;
            }
        }

        free(header);
    }
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
    close(socket);
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
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
        case 21: //ESI pide un GET
        {
            operation_get(message, socket);
            
            break;
        }
        case 22: //ESI pide un SET
        {
            log_info(logger, "ESI requested a SET");

            sleep(delay);
            //read(socket, message, header->len, 0);

            pthread_mutex_lock(&lock);
            int result = save_on_instance(instances);
            pthread_mutex_unlock(&lock);

            if(!result)
                operation = 33; 
                //avisar a planificador para abortar esi
                //supongo que tambien tengo que cerrar el hilo
            //else

            break;
        }
        case 30:
        {
            hello_id = SCHEDULER;
            log_info(logger,"Connected with the Scheduler");
            //agregar a una variable
            break;
        }
        case 31:
        {
            log_info(logger,"And key is not blocked");
            key_is_not_blocked = 1;
        }
        case 32:
        {
            log_info(logger,"But key is blocked");
            key_is_not_blocked = 0;
        }
        default:
        {
            log_error(logger, "Incorrect ID message");
            break;
        }
    }
}

void operation_get(content_header* header, int socket)
{
    sleep(delay);
    
    recv(socket, message, header->len, 0);

    log_info(logger, "ESI requested a GET of key: %s", message->key);
    sem_post(&esi_operation);

    sem_wait(&scheduler_response);
    
    pthread_mutex_lock(&lock);
    assign_instance(algorithm, instances);          
    pthread_mutex_unlock(&lock);
    
    send_answer(socket, key_is_not_blocked);  
}

void send_answer(int socket,int key_is_blocked)
{
    if(key_is_blocked)
    {
        send(socket, 22, sizeof(int), 0);
    }
    else
    {
        send(socket, 23, sizeof(int), 0);
    }
}

void assign_instance(_Algorithm algorithm, t_list* instances)
{
    instance_t* chosen_one;

    chosen_one = find_by_key(instances, message->key);
    
    if (chosen_one != NULL) //si me devuelve un elemento es porque ya se pidio antes
        return;

    switch(algorithm)
    {
        case EL:
        {
            chosen_one = find_by_times_used(instances);
        
            //guardar clave en lista
            
            break;
        }
    }
}

int save_on_instance(t_list* instances)
{
    instance_t* chosen_one = find_by_key(instances, message->key);

    if(chosen_one->is_active)
    {
        sem_post(&chosen_one->start);
        return 1;
    }
    else
    {
        log_warning(logger, "Instance has key but is not active");
        return 0;
    }
}

instance_t* add_instance_to_list(char* name, int socket)
{
    log_info(logger, "Received Instance name: %s", name);
    instance_t* inst_aux = (instance_t*)malloc(sizeof(instance_t));
    //agregar a diccionario de instancias
    inst_aux = name_is_equal(instances, name);
    if(inst_aux != NULL)
    { //si ya esta en la lista
        log_info(logger, "Updated state of: %s", name);
    }
    else
    { //si no esta en la lista
        inst_aux = (instance_t*)malloc(sizeof(instance_t)); 
        inst_aux->name = name;
        inst_aux->times_used = 0;
        inst_aux->space_used = 0;
        log_info(logger, "New Instance added to list");
    }

    inst_aux->socket = socket;
    inst_aux->is_active = 1;
    sem_init(&inst_aux->start, 0, 0);

    list_add(instances, inst_aux);

    return inst_aux;
}

void disconnect_instance_in_list(int socket)
{
    instance_t* instance = socket_is_equal(instances, socket);
    if(instance != NULL)
    {
        instance->is_active = 0;
        //pthread_mutex_unlock(&instance->start);
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

    return list_get(lista, 1);
}

instance_t* find_by_key(t_list* lista, char* key)
{
    bool _is_the_one(instance_t* p) 
    {
        //revisar lista de claves
        
        return string_equals_ignore_case(p->name, key);
    }

    return list_find(instances, _is_the_one);
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