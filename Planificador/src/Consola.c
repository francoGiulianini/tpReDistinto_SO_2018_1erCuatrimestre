#include "Planificador.h"
#include "Consola.h"

void Console(/*void *parameter*/)
{
	int exit = 0; //flag de hilo consola
    int consoleInputIndex = 0;
	char * line;
	char * command;
	char * key;
	char * id;

	printf("Type 'help' for a list of commands\n");
	while(exit == 0)
	{
		line = readline("Command: ");
		add_history(line);

		consoleInputIndex = 0;

		command = consoleReadArg(line, &consoleInputIndex);

		switch(to_enum(command))
		{
			case EXIT:
			{
				stop = 1;
				exit = 1;
				sem_post(&hay_esis);
				pthread_mutex_unlock(&pause_mutex);
				log_warning(logger, "The Scheduler was closed by command");
				break;
			}
			case PAUSE:
			{
				//el panificador se pone en pausa
				pthread_mutex_lock(&pause_mutex);
				log_info(logger, "The Scheduler was paused by command");
				break;
			}
			case RESUME:
			{
				//el panificador vuelve de la pausa
				pthread_mutex_unlock(&pause_mutex);
				log_info(logger, "The Scheduler was unpaused by command");
				break;
			}
			case BLOCK:
			{
				//bloquear ESI
				key = consoleReadArg(line, &consoleInputIndex);
				if(null_argument(key, "<key>"))
					break;

				id = consoleReadArg(line, &consoleInputIndex);
				if(null_argument(id, "<id>"))
					break;

				block_key_by_console(key, id);

				log_info(logger, "The Scheduler blocked key: %s with id: %s by command", key, id);
				free(key);
				free(id);
				break;
			}
			case UNLOCK:
			{
				//desbloquear clave
				key = consoleReadArg(line, &consoleInputIndex);
				if(null_argument(key, "<key>"))
					break;
				log_info(logger, "The Scheduler unlocked the key: %s by command", key);
				unlock_key_by_console(key);
				free(key);
				break;
			}
			case LIST:
			{
				//mostrar los esi bloqueados por el recurso
				key = consoleReadArg(line, &consoleInputIndex);
				if(null_argument(key, "<key>"))
					break;

				list_blocked_esis(key);
				free(key);
				break;
			}
			case KILL:
			{
				//finaliza el esi
				id = consoleReadArg(line, &consoleInputIndex);
				log_info(logger, "ESI with id: %s terminated by command",id);
				free(id);
				break;
			}
			case STATUS:
			{
				//ver enunciado
				key = consoleReadArg(line, &consoleInputIndex);
				if(null_argument(key, "<key>"))
					break;
				log_info(logger, "Status information displayed by command");

				pthread_mutex_lock(&status_mutex);
				get_key_status(key);
				pthread_mutex_unlock(&status_mutex);

				free(key);
				break;
			}
			case DEADLOCK:
			{
				//deadlock
				log_info(logger, "Deadlock by command");
				break;
			}
			case HELP:
			{
				//lista de todos los commandos
				printf("exit\npause\nresume\nblock <id> <key>\nunlock <key>\n"
				"list <resource>\nkill <id>\nstatus <key>\ndeadlock\n");
				break;
			}
			case ERROR:
			{
				printf("Invalid Command\n");
				break;
			}
		}
		
		free(line);
		free(command);
	}
	
}

char * consoleReadArg(char * source, int * i)
{
	char * result = (char *)malloc((strlen(source) + 1) * sizeof(char));
	int j = 0;

	while(source[*i] == ' ')
	{
		*i = *i + 1;
	}

	while(source[*i] != ' ' && source[*i] != '\0')
	{
		result[j] = source[*i];
		*i = *i + 1;
		j++;
	}

	result[j] = '\0';

	return result;
}

_Commands to_enum(char * source)
{
	if (string_equals_ignore_case(source, "EXIT"))
		return EXIT;
	if (string_equals_ignore_case(source, "PAUSE"))
		return PAUSE;
	if (string_equals_ignore_case(source, "RESUME"))
		return RESUME;
	if (string_equals_ignore_case(source, "BLOCK"))
		return BLOCK;
	if (string_equals_ignore_case(source, "UNLOCK"))
		return UNLOCK;
	if (string_equals_ignore_case(source, "LIST"))
		return LIST;	
	if (string_equals_ignore_case(source, "KILL"))
		return KILL;
	if (string_equals_ignore_case(source, "STATUS"))
		return STATUS;
	if (string_equals_ignore_case(source, "DEADLOCK"))
		return DEADLOCK;
	if (string_equals_ignore_case(source, "HELP"))
		return HELP;					
	return ERROR;
}

int null_argument(char* arg_console, char* for_logger)
{
	if(strcmp(arg_console,"") == 0)
	{
		log_warning(logger, "Missing argument: %s", for_logger);
		return 1;
	}
	return 0;
}

void block_key_by_console(char * key, char * id)
{
	/*sem_wait(&esi_executing);
	pthread_mutex_lock(&pause_mutex);
	//ver si esta ejecutando o en listo
	t_esi * otro_esi = find_and_remove_by_id(lista_ready, id);
	
	clave_bloqueada_t* un_key = find_by_key(lista_bloqueados, key);

	//si la clave no existe crearla(ver en planificador.c)

	if(!otro_esi == NULL) //esta en cola listo
	{	
		//otro_esi->console_blocked = 1;
		queue_push(un_key->cola_esis_bloqueados, otro_esi);
	}	
	else	//esta en ejecucion
	{
		queue_push(un_key->cola_esis_bloqueados, un_esi);
		blocked_esi_by_console = 1;
		fin_de_esi = 1;
	}

	list_add(lista_bloqueados, un_key);
	pthread_mutex_unlock(&pause_mutex);*/
}

void unlock_key_by_console(char* key)
{
	void _log_all_ready_b(t_esi* p)
	{
		log_info(logger, "In Ready queue before unlocking: %p", p->name);
	}
	list_iterate(lista_ready, _log_all_ready_b);

	clave_bloqueada_t* a_key = (clave_bloqueada_t*)malloc(sizeof(clave_bloqueada_t));
	a_key = find_by_key(lista_bloqueados, key);

	if(a_key == NULL)
	{
		printf("Requested Key doesnt exist\n");
		return;
	}

	bool _is_the_key(clave_bloqueada_por_esi_t * p)
	{
		log_info(logger, "key blocked: %s key to unlock: %s", p->key, key);
		return string_equals_ignore_case(p->key, key);
	}

	clave_bloqueada_por_esi_t * blocked_key = list_remove_by_condition(claves_bloqueadas_por_esis, _is_the_key);
    if(blocked_key != NULL)
    {
    	free(blocked_key->esi_id);
    	free(blocked_key->key);
    	free(blocked_key);
    }

	if(queue_is_empty(a_key->cola_esis_bloqueados))
	{
		return;
	}

	t_esi* otro_esi = queue_pop(a_key->cola_esis_bloqueados);
	if(string_equals_ignore_case(otro_esi->name, "CONSOLA"))
	{
		if(queue_is_empty(a_key->cola_esis_bloqueados))
		{
			return;
		}

		otro_esi = queue_pop(a_key->cola_esis_bloqueados);
	}

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
        case SJFCD:         
        {
            calculate_estimation(otro_esi);		//revisar estimaciones de un_esi y el que se libera

            log_info(logger, "Estimation for: %s is: %f",
                otro_esi->name, otro_esi->cpu_time_estimated);


            if (otro_esi->cpu_time_estimated < un_esi->cpu_time_estimated)
            {
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

            otro_esi->waiting_time = (float)0;
            list_add(lista_ready, otro_esi);
            
            break;
        }
    }
    //sort_list_by_algorithm(lista_ready);		//Independientemente de los casos, reordeno lista_ready
    
	void _log_all_ready_a(t_esi* p)
	{
		log_info(logger, "In Ready queue after unlocking: %s", p->name);
	}
	list_iterate(lista_ready, _log_all_ready_a);
	sem_post(&hay_esis);
}

void list_blocked_esis(char* resource)
{
	clave_bloqueada_t* a_key = (clave_bloqueada_t*)malloc(sizeof(clave_bloqueada_t));
	a_key = find_by_key(lista_bloqueados, resource);

	if(a_key == NULL)
	{
		printf("Requested Key doesnt exist\n");
		return;
	}
	
	if(queue_is_empty(a_key->cola_esis_bloqueados))
	{
		printf("No ESIs are blocked\n");
		return;
	}
	
	printf("ESIS Blocked waiting for: %s\n", resource);
	int cant = queue_size(a_key->cola_esis_bloqueados);
	for(int i = 1; i <= cant; i++)
	{
		t_esi* otro_esi = queue_pop(a_key->cola_esis_bloqueados);
		printf("%s\n", otro_esi->name);
		queue_push(a_key->cola_esis_bloqueados, otro_esi);
	}
}

void get_key_status(char* key)
{
	clave_bloqueada_t* a_key = (clave_bloqueada_t*)malloc(sizeof(clave_bloqueada_t));
	a_key = find_by_key(lista_bloqueados, key);
	
	if(a_key == NULL)//la clave nunca se pidio, entonces hay que simular
	{	
		printf("Requested Key doesnt exist\n");

		//simular asignacion de instancia
		send_header(socket_c, 33);

		send(socket_c, key, strlen(key), 0);
		content_header* header = malloc(sizeof(content_header));

		recv(socket_c, header, sizeof(content_header), 0);
		if(header->id == 36)//instancia simulada
		{
			char * instance_name = malloc(header->len + 1);
			recv(socket_c, instance_name, header->len, 0);
			instance_name[header->len] = '\0';

			printf("Key should get assigned to: %s\n", instance_name);

			free(instance_name);
			free(header);
		}
		return;
	}
	else//la clave fue pedida, recibir valor o instancia en la que está
	{
		send_header(socket_c, 33);

		content_header* header = malloc(sizeof(content_header));

		recv(socket, header, sizeof(content_header), 0);

		if(header->id == 37) //tiene valor asignado
		{
			char * value = malloc(header->len2 + 1);
			char * instance_name = malloc(header->len + 1);
			char * message_recv = malloc(header->len + header->len2 + 1);

			recv(socket_c, message_recv, header->len + header->len2, 0);

			memcpy(instance_name, message_recv, header->len);
			memcpy(value, message_recv + header->len, header->len2);
			instance_name[header->len] = '\0';
			value[header->len2] = '\0';

			printf("Instance: %s Value: %s\n",instance_name, value);

			free(instance_name);
			free(value);
			free(message_recv);
		}
		if(header->id == 38) //no tiene valor o fue reemplazada
		{
			char * instance_name = malloc(header->len + 1);
			recv(socket_c, instance_name, header->len, 0);
			instance_name[header->len] = '\0';

			printf("Instance: %s Does not have a value\n", instance_name);

			free(instance_name);
		}

		free(header);

		return;			
	}
}

t_esi * find_by_id(t_list * lista, char* id)
{
	bool _is_this_one(t_esi* p)
	{
		return string_equals_ignore_case(p->name, id);
	}

	return list_find(lista, _is_this_one);
}

t_esi * find_and_remove_by_id(t_list * lista, char* id)
{
	bool _is_this_one(t_esi* p)
	{
		return string_equals_ignore_case(p->name, id);
	}

	return list_remove_by_condition(lista, _is_this_one);
}

clave_bloqueada_t * find_by_key(t_list * lista, char* key)
{
	bool _is_this_the_one(clave_bloqueada_t* p)
	{
		return string_equals_ignore_case(p->key, key);
	}

	return list_find(lista, _is_this_the_one);
}