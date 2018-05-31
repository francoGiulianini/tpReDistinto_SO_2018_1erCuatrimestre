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
int just_disconnected = 0;
int instance_pointer = 0;
int key_is_not_blocked = 1;
int num_esi = 0;
int result = 1;

int main(void) 
{
	pthread_mutex_init(&lock, NULL);
    sem_init(&one_instance, 0, 0);
    sem_init(&one_esi, 0, 0);
    sem_init(&esi_operation, 0, 0);
    sem_init(&scheduler_response, 0, 0);
    sem_init(&result_get, 0, 0);
    sem_init(&result_set, 0, 0);

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

    config_instance = malloc(sizeof(config_instance_t));

    int aux_number;
    if(config_has_property(config, "CantidadEntradas"))
    {
        aux_number = config_get_int_value(config, "CantidadEntradas");
        log_info(logger, "Number of Entries from config file: %d", aux_number);
    }
    else
        exit_with_error(logger, "Cannot read Number of Entries from config file");

    int aux_size;
    if(config_has_property(config, "TamanioEntrada"))
    {
        aux_size = config_get_int_value(config, "TamanioEntrada");
        log_info(logger, "Size of Entries from config file: %d", aux_size);
    }
    else
        exit_with_error(logger, "Cannot read Delay from config file");

    config_instance->number = aux_number;
    config_instance->size = aux_size;
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

    send(socket, config_instance, sizeof(config_instance_t), 0);

    sem_post(&one_instance);

    content_header* header = malloc(sizeof(content_header));

    while(1)
    {
        sem_wait(&instance->start);

        switch(instance_operation)
        {
            case GET:
                //preguntar si la instancia sigue viva               
                header->id = 13;

                send(socket, header, sizeof(content_header), 0);

                if ((valread = recv(socket , header, sizeof(content_header), 0)) == 0)
                {    
                    disconnect_socket(socket, true);

                    just_disconnected = 1;
                }
                else
                    just_disconnected = 0;

                sem_post(&result_get);
                break;
            case SET:
                int len1 = strlen(message->key);
                int len2 = strlen(message->value);

                header->id = 12;
                header->len = len1;
                header->len2 = len2;

                log_warning(logger, "Sending Value to Instance: %s", name);
                send(socket, header, sizeof(content_header), 0);
                
                //serializar message
                int len_message1 = strlen(message->key);
                int len_message2 = strlen(message->value);
                char * message_send = malloc(len_message1 + len_message2);
                memcpy(message_send, message->key, len_message1);
                memcpy(message_send + len_message1, message->value, len_message2);

                send(socket, message_send, len1 + len2, 0);

                if ((valread = recv(socket , header, sizeof(content_header), 0)) == 0)
                    disconnect_socket(socket, true);

                process_message_header(header, socket);

                sem_post(&result_set);
                free(message_send);
                break;
            case STATUS:
                break;
        }
    }

    free(header);
}

void host_esi(void* arg)
{
    thread_data_t *data = (thread_data_t *) arg;
    int socket = data->socket;
    int len = data->next_message_len;

    //asignar un nombre
    num_esi++;

    char* name = (char*)malloc(7);
    if(num_esi < 10)
        sprintf(name, "ESI0%d", num_esi);
    else 
        sprintf(name, "ESI%d", num_esi);

    log_info(logger, "ESI ID: %s", name);

    t_dictionary * blocked_keys_by_this_esi = dictionary_create();

    sem_post(&one_esi);

    int valread;
    content_header* header = malloc(sizeof(content_header));

    while(1)
    {
        if ((valread = recv(socket ,header, sizeof(content_header), 0)) == 0)
            disconnect_socket(socket, false);
        
        process_message_header_esi(header, socket, blocked_keys_by_this_esi, name);
    }

    free(header);
}

void host_scheduler(void* arg)
{
    thread_data_t *data = (thread_data_t *) arg;
    int socket = data->socket;
    int len = data->next_message_len;
    int valread;

    sem_wait(&one_esi);
    sem_wait(&one_instance);
    send_header(socket, 35);

    while(1)
    {
        content_header* header = malloc(sizeof(content_header));
        //int key_id;

        //for STATUS operation
        recv(socket, header, sizeof(content_header), MSG_DONTWAIT);
        if(header->id == STATUS)
        {
            //abrir hilo para status o hacerlo aca
        }

        sem_wait(&esi_operation);
        
        header->len = strlen(message->key);
        switch(operation)
        {
            case GET: //preguntar por clave
            {
                log_info(logger, "Checking key with scheduler");
                header->id = GET;
                send(socket, header, sizeof(content_header), 0);
                
                send(socket, message->key, header->len, 0);

                recv(socket, header, sizeof(content_header), 0);

                process_message_header(header, socket);
                sem_post(&scheduler_response);
                break;
            }
            case 32: //desbloquear clave
            {
                break;
            }
            case SET: //operacion set
            {
                header->id = SET;
                send(socket, header, sizeof(content_header), 0);

                break;
            }
            case 34: //abortar esi
            {
                header->id = 34;
                send(socket, header, sizeof(content_header), 0);

                break;
            }
        }

        free(header->id);
        free(header->len);
        free(header->len2);
        free(header);
    }
}

void disconnect_socket(int socket, bool is_instance)
{
    //disconnected , get his details and print   
    if(is_instance)
        {
            pthread_mutex_lock(&lock);
            disconnect_instance_in_list(socket);
            pthread_mutex_unlock(&lock);
        }
    
    getpeername(socket , (struct sockaddr*)&serverAddress , (socklen_t*)&addrlen);  
    log_info(logger, "Host disconnected , ip %s , port %d", 
        inet_ntoa(serverAddress.sin_addr) , ntohs(serverAddress.sin_port));  

    close(socket);
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
        case 11:    //compactar
        {
            initiate_compactation();

            break;
        }
        case 12: //todo ok
        {
            break;
        }
        case 20:
        {
            hello_id = ESI;
            log_info(logger,"Connected with an ESI");
            //agregar a vector de esis
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
            break;
        }
        case 32:
        {
            log_info(logger,"But key is blocked");
            key_is_not_blocked = 0;
            break;
        }
        case STATUS:
        {
            break;
        }
        default:
        {
            log_error(logger, "Incorrect ID message");
            break;
        }
    }
}

void process_message_header_esi(content_header* header, int socket, t_dictionary * blocked_keys, char* name)
{
    switch(header->id)
    {
        case 21: //ESI pide un GET
        {
            operation_get(header, socket, blocked_keys, name);
            
            break;
        }
        case 22: //ESI pide un SET
        {
            operation_set(header, socket, blocked_keys, name);

            break;
        }
        case 23: //ESI pide un STORE
        {
            break;
        }
    }
}

void operation_get(content_header* header, int socket, t_dictionary * blocked_keys, char* name)
{
    usleep(delay);

    message = (message_content*) malloc(header->len + header->len2);
    char* message_recv = malloc(header->len + header->len2);

    int result = recv(socket, message_recv, header->len + 1, 0);
    if(result <= 0)
        perror("Recv:");

    message->key = message_recv;

    log_info(logger, "%s requested a GET of key: %s", name, message->key);
    operation = GET;
    instance_operation = GET;
    sem_post(&esi_operation);

    sem_wait(&scheduler_response);
    
    pthread_mutex_lock(&lock);
    assign_instance(algorithm, instances);          
    pthread_mutex_unlock(&lock);
    
    send_answer(socket, key_is_not_blocked);

    dictionary_put(blocked_keys, message->key, NULL);

    free(message->key);
    free(message->value);
    free(message);
    free(message_recv);
}

void operation_set(content_header * header, int socket, t_dictionary * blocked_keys, char* name)
{
    usleep(delay);
    
    char* message_recv = malloc(header->len + header->len2);
    message = (message_content*)malloc(sizeof(char) * header->len + header->len2);

    recv(socket, message_recv, header->len + header->len2, 0);

    message->key = malloc(header->len);
    message->value = malloc(header->len2);
    memcpy(message->key, message_recv, header->len);
    memcpy(message->value, message_recv + header->len, header->len2);

    log_info(logger, "%s requested a SET of Key: %s, with Value: %s", name, message->key, message->value);

    if(!dictionary_has_key(blocked_keys, message->key))
    {
        log_info(logger, "But Key was not requested before. Aborting ESI");
        abort_esi(socket);
    }

    pthread_mutex_lock(&lock);
    result = save_on_instance(instances);
    pthread_mutex_unlock(&lock);

    operation = SET;
    sem_post(&esi_operation);

    sem_wait(&result_set);

    if(!result)
    {
        log_info(logger, "But Instance was not available. Aborting ESI");
        abort_esi(socket);
    }
    else
    {
        log_info(logger, "Operation Successful");
        send_header(socket, 23); //operacion con exito
    }

    free(message->key);
    free(message->value);
    free(message);
    free(message_recv); 
}

void initiate_compactation()
{
    void _send_to_instance(instance_t * i)
    {
        send_header(i->socket, 11);
    }

    list_iterate(instances, _send_to_instance);
}

void abort_esi(int socket)
{
    operation = 33;
    sem_post(&esi_operation);
    send_header(socket, 24);
    disconnect_socket(socket, false);
}

void send_answer(int socket,int key_is_not_blocked)
{
    if(!key_is_not_blocked)
    {
        send_header(socket, 22);
    }
    else
    {
        send_header(socket, 23);
    }
}

void send_header(int socket, int id)
{
    content_header* header = malloc(sizeof(content_header));
    header->id = id;
        
    send(socket, header, sizeof(content_header), 0);
}

void assign_instance(_Algorithm algorithm, t_list* instances)
{
    instance_t* chosen_one;

    chosen_one = find_by_key(instances, message->key);
    
    if ((chosen_one != NULL) || (chosen_one->is_active)) //si me devuelve un elemento es porque ya se pidio antes
    {    
        sem_post(&chosen_one->start);
        
        sem_wait(&result_get);
        if(just_disconnected == 0)
            return;
    }

    switch(algorithm)
    {
        case EL:
        {
            chosen_one = choose_by_counter(instances);

            list_add(chosen_one->keys, message->key);
            
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

instance_t* find_by_space_used(t_list* lista)
{
    t_list* list_aux = list_create();
    
    bool _is_active(instance_t* p) 
    {
        if(p->is_active == 1)
            return true;
        else
            return false;
    }
    bool _lower_than_the_next(instance_t* p, instance_t* q) 
    {
        if(p->space_used < q->space_used)
            return true;
        else
            return false;
    }
    t_list* list_aux;

    list_aux = list_filter(lista, _is_active);
    list_sort(list_aux, _lower_than_the_next);

    return list_get(list_aux, 1);
}

instance_t* choose_by_counter(t_list* lista)
{
    int i_max = list_size(lista) - 1;

    instance_t* an_instance = list_get(lista, instance_pointer++);

    while((an_instance->is_active == 0) && (i_max >= instance_pointer))
        an_instance = list_get(lista, instance_pointer++);

    if(i_max < instance_pointer)
    {
        instance_pointer = 0;
    }

    return an_instance;
}

instance_t* find_by_key(t_list* lista, char* key)
{
    bool _is_the_one(instance_t* p) 
    {
        bool _has_the_key(char* q)
        {
            return string_equals_ignore_case(q, key);
        }

        if(list_find(p->keys, _has_the_key) == NULL)
            return false;
        else
            return true;
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