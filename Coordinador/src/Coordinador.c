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

int error;
_Operation operation;
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
	sem_init(&result_store, 0, 0);

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
        log_info(logger, "Delay(in miliseconds) from config file: %d", delay);
        delay_req.tv_sec = 0;
        delay_req.tv_nsec = (long int)delay * 1000 * 1000;
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
    name[len] = '\0';

    instance_t * instance;
    instance = malloc(sizeof(instance_t));

    instance = add_instance_to_list(name, socket);

    config_instance_t* config_aux = malloc(sizeof(config_instance_t));
    config_aux->number = config_instance->number;
    config_aux->size = config_instance->size;
    send(socket, config_aux, sizeof(config_instance_t), 0);

    sem_post(&one_instance);

    content_header* header = malloc(sizeof(content_header));

    while(1)
    {
        sem_wait(&instance->start);

        switch(operation)
        {
            case GET:
            {    //preguntar si la instancia sigue viva               
                header->id = 13;
                header->len = strlen(message->key);
                header->len2 = 0;
                send(socket, header, sizeof(content_header), 0);

                /*if ((valread = recv(socket , header, sizeof(content_header), 0)) == 0)
                {    
                    disconnect_socket(socket, true);

                    just_disconnected = 1;
                }
                else
                    just_disconnected = 0;*/

                send(socket, message->key, strlen(message->key) + 1, 0);

                //esperar confirmacion de la instancia
                recv(socket, header, sizeof(content_header), 0);    

                log_info(logger, "Instance finished GET");

                sem_post(&result_get);
                break;
            }
            case SET:
            {
                int length1 = strlen(message->key) + 1;
                int length2 = strlen(message->value) + 1;

                header->id = 12;
                header->len = length1;
                header->len2 = length2;

                log_warning(logger, "Sending Value to Instance: %s", name);
                send(socket, header, sizeof(content_header), 0);
                
                //serializar message
                char * message_send = malloc(length1 + length2);
                memcpy(message_send, message->key, length1);
                memcpy(message_send + length1, message->value, length2);

                send(socket, message_send, length1 + length2, 0);

                if ((valread = recv(socket , header, sizeof(content_header), 0)) == 0)
                    disconnect_socket(socket, true);

                process_message_header(header, socket);

				//actualizar datos de la instancia
				update_instance(instance, header);

                free(message_send);

                log_info(logger, "Instance finished SET");

                sem_post(&result_set);
                break;
            }
			case STORE:
			{
				header->id = 14;
				header->len = strlen(message->key);
				header->len2 = 0;
				send(socket, header, sizeof(content_header), 0);

				send(socket, message->key, strlen(message->key) + 1, 0);

				recv(socket, header, sizeof(content_header), 0);

				log_info(logger, "Instance finished STORE");

				sem_post(&result_store);

				break;
			}
            case STATUS:
                break;
            default:
            {
                log_error(logger, "Oops...");
            }    
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
        if(header->id == 39)
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
                header->id = 31;
                header->len2 = 0;
                send(socket, header, sizeof(content_header), 0);
                
                send(socket, message->key, header->len, 0);

                valread = recv(socket, header, sizeof(content_header), 0);
				if (valread <= 0)
				{
					exit_with_error(logger, "Scheduler Disconnected");
				}

                process_message_header(header, socket);
                sem_post(&scheduler_response);
                break;
            }
            case STORE: //desbloquear clave
            {
                log_info(logger, "Unlocking key with scheduler");
                header->id = 32;
                header->len2 = 0;
                send(socket, header, sizeof(content_header), 0);

                send(socket, message->key, header->len, 0);

                valread = recv(socket, header, sizeof(content_header), 0);
				if (valread <= 0)
				{
					exit_with_error(logger, "Scheduler Disconnected");
				}

				process_message_header(header, socket);

                sem_post(&scheduler_response);
                break;
            }
            case SET: //operacion set
            {
                header->id = 33;
                header->len2 = 0;
                send(socket, header, sizeof(content_header), 0);

				send(socket, message->key, header->len, 0);

				valread = recv(socket, header, sizeof(content_header), 0);
				if (valread <= 0)
				{
					exit_with_error(logger, "Scheduler Disconnected");
				}

				process_message_header(header, socket);

				sem_post(&scheduler_response);
                break;
            }
            case ABORT: //abortar esi
            {
                header->id = 34;
                header->len = 0;
                header->len2 = 0;
                send(socket, header, sizeof(content_header), 0);

                break;
            }
        }

        ////free(header->id);
        ////free(header->len);
        ////free(header->len2);
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
			//nothing

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
        case 34:
        {
			log_info(logger, "ESI has the key");
			key_is_not_blocked = 1;
            break;
        }
		case 35:
		{
			log_info(logger, "ESI does not have the key anymore");
			key_is_not_blocked = 0;
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
            operation_store(header, socket, blocked_keys, name);

            break;
        }
    }
}

void operation_get(content_header* header, int socket, t_dictionary * blocked_keys, char* name)
{
    char* message_recv = (char*)malloc(header->len + header->len2);
    message = (message_content*) malloc(sizeof(message_content));
    message->key = malloc(header->len + 1);

    recv(socket, message_recv, header->len, 0);
    memcpy(message->key, message_recv, header->len);
    message->key[header->len] = '\0';

    log_info(logger, "%s requested a GET of key: %s", name, message->key);
    //retardo de operaciones
    nanosleep(&delay_req, &time_rem);

    //flag para hilos instancia y planificador
    operation = GET;
    sem_post(&esi_operation);

    sem_wait(&scheduler_response);
    
    if(!key_is_not_blocked)
    {       
        send_header(socket, 22);
    }    
    else
    {    
        send_header(socket, 23);

        pthread_mutex_lock(&lock);
        assign_instance(algorithm, instances);          
        pthread_mutex_unlock(&lock);
        
        dictionary_put(blocked_keys, message->key, NULL);
    } 
  
    //free(message->key);
    //free(message->value);
    ////free(message);
    ////free(message_recv);
}

void operation_set(content_header * header, int socket, t_dictionary * blocked_keys, char* name)
{  
    char* message_recv = malloc(header->len + header->len2);
    message = (message_content*)malloc(sizeof(message_content));
    message->key = malloc(header->len + 1);
    message->value = malloc(header->len2 + 1);

    recv(socket, message_recv, header->len + header->len2, 0);

    //deserializacion    
    memcpy(message->key, message_recv, header->len);
    message->key[header->len] = '\0';
    memcpy(message->value, message_recv + header->len, header->len2);
    message->value[header->len2] = '\0';

    log_info(logger, "%s requested a SET of Key: %s, with Value: %s", name, message->key, message->value);
    //retardo de operaciones
    nanosleep(&delay_req, &time_rem);

    if(!dictionary_has_key(blocked_keys, message->key))
    {
        log_info(logger, "But Key was not requested before. Aborting ESI");
        abort_esi(socket);
        return;
    }

    //flag para hilos instancia y planificador
    operation = SET;

	sem_post(&esi_operation);

	if (key_is_not_blocked)
	{
		pthread_mutex_lock(&lock);
		result = save_on_instance(instances);
		pthread_mutex_unlock(&lock);
	}
	else
	{
		//esi tiene que abortar
		abort_esi(socket);
		return;
	}

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

    //free(message->key);
    //free(message->value);
    //free(message);
    //free(message_recv); 
}

void operation_store(content_header* header, int socket, t_dictionary * blocked_keys, char* name)
{
    message = (message_content*) malloc(sizeof(message_content));
    char* message_recv = malloc(header->len + header->len2);

    int result = recv(socket, message_recv, header->len, 0);
    if(result <= 0)
        perror("Recv:");

    //deserealizacion
    message->key = malloc(header->len + 1);
    memcpy(message->key, message_recv, header->len);
    message->key[header->len] = '\0';

    log_info(logger, "%s requested a STORE of key: %s", name, message->key);
    //retardo de operacion
    nanosleep(&delay_req, &time_rem);

    if(!dictionary_has_key(blocked_keys, message->key))
    {
        log_info(logger, "But Key was not requested before. Aborting ESI");
        abort_esi(socket);
        return;
    }

    //flag para hilos instancia y planificador
	operation = STORE;

    sem_post(&esi_operation);

	//buscar instancia con la clave y avisarle para que guarde en disco
	if (key_is_not_blocked)
	{
		pthread_mutex_lock(&lock);
		result = save_on_instance(instances);
		pthread_mutex_unlock(&lock);
	}
	else
	{
		//esi tiene que abortar
		abort_esi(socket);
		return;
	}  

	sem_wait(&result_store);

	if (!result)
	{
		log_info(logger, "But Instance was not available. Aborting ESI");
		abort_esi(socket);
		return;
	}
	else
	{
		log_info(logger, "Operation Successful");
		send_header(socket, 23); //operacion con exito
		dictionary_remove(blocked_keys, message->key);
	}
}

void update_instance(instance_t* one_instance, content_header* header)
{
	switch (algorithm)
	{
	case EL:
	{
		//nothing
		break;
	}
	case LSU:
	{
		//actualizar espacios libres

		one_instance->free_space = header->len;
		break;
	}
	case KE:
	{
		//nothing
		break;
	}
	}
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
    operation = ABORT;
    sem_post(&esi_operation);
    send_header(socket, 24);
    disconnect_socket(socket, false);
}

void send_header(int socket, int id)
{
    content_header* header = malloc(sizeof(content_header));
    header->id = id;
    header->len = 0;
    header->len2 = 0;
        
    send(socket, header, sizeof(content_header), 0);
}

void assign_instance(_Algorithm algorithm, t_list* instances)
{
    instance_t* chosen_one = malloc(sizeof(instance_t));

    chosen_one = find_by_key(instances, message->key);
    
    if (chosen_one != NULL) //si me devuelve un elemento es porque ya se pidio antes
    {    
        if(chosen_one->is_active)
        {
            sem_post(&chosen_one->start);
            
            sem_wait(&result_get);
            if(just_disconnected == 0)
                return;
        }
    }

	chosen_one = malloc(sizeof(instance_t));

    switch(algorithm)
    {
        case EL:
        {
            chosen_one = choose_by_counter(instances);

            break;
        }
		case LSU:
		{
			chosen_one = choose_by_space(instances);

			break;
		}
		case KE:
		{
			chosen_one = choose_by_letter(instances);

			break;
		}
    }

	log_warning(logger, "%s was chosen to store key: %s", chosen_one->name, message->key);
	dictionary_put(chosen_one->keys, message->key, NULL);

	sem_post(&chosen_one->start);
	sem_wait(&result_get);
}

int save_on_instance(t_list* instances)
{
    instance_t* chosen_one = find_by_key(instances, message->key);

    if(chosen_one->is_active)
    {
        sem_post(&chosen_one->start);
        log_info(logger, "Saving Key on %s", chosen_one->name);
        return 1;
    }
    else
    {
        log_warning(logger, "Instance has key but is not active");
        return 0;
    }
}

/*int store_on_instance(t_list* instances)
{
	instance_t* chosen_one = find_by_key(instances, message->key);
}*/

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
        inst_aux->free_space = 0;
		inst_aux->letter_min = 0;
		inst_aux->letter_max = 0;
        inst_aux->keys = dictionary_create();
        log_info(logger, "New Instance added to list");
    }

    inst_aux->socket = socket;
    inst_aux->is_active = 1;
    sem_init(&inst_aux->start, 0, 0);

    list_add(instances, inst_aux);

	if (algorithm == KE)
		assign_letters();

    return inst_aux;
}

void assign_letters()
{
	bool _is_active(instance_t* p)
	{
		return p->is_active;
	}

	int cant_letters = 25;

	int cant_instances = list_count_satisfying(instances, _is_active);

	int letters_to_instances = cant_letters / cant_instances;
	int rest_letters = cant_letters % cant_instances;
    if(rest_letters != 0)
        letters_to_instances++;
	int assigned_letters = 0;

	void _assign_letters(instance_t* p)
	{
		if (p->is_active)
		{
			if (letters_to_instances <= cant_letters)
			{
				p->letter_min = 65 + assigned_letters;
				p->letter_max = 65 + assigned_letters + letters_to_instances;
				log_info(logger, "%s has keys starting from: %c, to: %c", p->name, p->letter_min, p->letter_max);
				assigned_letters = assigned_letters + letters_to_instances + 1;
				cant_letters -= letters_to_instances;
			}
			else
			{
				p->letter_min = 65 + assigned_letters;
				p->letter_max = 65 + assigned_letters + cant_letters - 1;
				log_info(logger, "%s has keys starting from: %c, to: %c", p->name, p->letter_min, p->letter_max);
			}
		}
	}

    list_iterate(instances, _assign_letters);
}

void disconnect_instance_in_list(int socket)
{
    instance_t* instance = socket_is_equal(instances, socket);
    if(instance != NULL)
    {
        instance->is_active = 0;
		instance->letter_min = 0;
		instance->letter_max = 0;

		if(algorithm == KE)
			assign_letters();

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

instance_t* choose_by_space(t_list* lista)
{
    instance_t* an_instance = find_by_free_space(lista);

    return an_instance;
}

instance_t* choose_by_letter(t_list* lista)
{
	char letter = message->value[0];
	if (letter >= 97 && letter <= 122) //si esta en minuscula se pasa a mayuscula
		letter -= 32;

	if (letter == 164 || letter == 165) //si es una Ñ se pasa a N
		letter = 78;

	instance_t* an_instance = find_by_letter(lista, letter);

	return an_instance;
}

instance_t* find_by_key(t_list* lista, char* key)
{
    bool _is_the_one(instance_t* p) 
    {
        dictionary_has_key(p->keys, key);
    }

    return list_find(instances, _is_the_one);
}

instance_t* find_by_free_space(t_list* lista)
{
	t_list* list_aux = list_create();

	bool _is_active(instance_t* p)
	{
		if (p->is_active == 1)
			return true;
		else
			return false;
	}
	bool _lower_than_the_next(instance_t* p, instance_t* q)
	{
		if (p->free_space > q->free_space)
			return true;
		else
			return false;
	}


	list_aux = list_filter(lista, _is_active);
	list_sort(list_aux, _lower_than_the_next);

	instance_t* one_instance = list_remove(list_aux, 0);

	list_destroy(list_aux); //ver si la otra lista sigue funcionando

	return one_instance;
}

instance_t* find_by_letter(t_list* lista, char letter)
{
	bool _has_letter(instance_t* p)
	{
		if (p->letter_min <= letter && p->letter_max >= letter)
		{
			return true;
		}
		return false;
	}

	return list_find(lista, _has_letter);
}

_Algorithm to_algorithm(char* string)
{
    if(string_equals_ignore_case(string, "EL"))
        return EL;
    if(string_equals_ignore_case(string, "LSU"))
        return LSU;
    if(string_equals_ignore_case(string, "KE"))
        return KE;
    log_warning(logger, "Incorrect algorithm type, using default EL");
    return EL;
}