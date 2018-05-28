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
	char * resource;

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

				block_esi_by_console(key, id);

				log_info(logger, "The Scheduler blocked an ESI with key: %s and id: %s by command", key, id);
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
				free(key);
				break;
			}
			case LIST:
			{
				//mostrar los esi bloqueados por el recurso
				resource = consoleReadArg(line, &consoleInputIndex);
				if(null_argument(key, "<key>"))
					break;

				list_blocked_esis(resource);
				free(resource);
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
				//da informacion de las instancias?
				key = consoleReadArg(line, &consoleInputIndex);
				log_info(logger, "Status information displayed by command");
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

void block_esi_by_console(char * key, char * id)
{
	sem_wait(&esi_executing);
	pthread_mutex_lock(&pause_mutex);
	//ver si esta ejecutando o en listo
	t_esi * otro_esi = find_by_id(cola_ready, id);
	
	clave_bloqueada_t* un_key = find_by_key(lista_bloqueados, key);

	if(!otro_esi == NULL) //esta en cola listo
		queue_push(un_key->cola_esis_bloqueados, otro_esi);
	else	//esta en ejecucion
	{
		queue_push(un_key->cola_esis_bloqueados, un_esi);
		blocked_esi_by_console = 1;
	}

	list_add(lista_bloqueados, un_key);
	pthread_mutex_unlock(&pause_mutex);
}

void list_blocked_esis(char* resource)
{
	clave_bloqueada_t* a_key = find_by_key(lista_bloqueados, resource);

	if(a_key == NULL)
	{
		log_warning(logger, "Requested Key doesnt exist");
	}

	void _log_all_esis(t_esi* an_esi)
	{
		printf("%s\n", an_esi->name);
	}

	printf("ESIS Blocked waiting for: %s", resource);
	list_iterate(a_key->cola_esis_bloqueados, _log_all_esis);
}

t_esi * find_by_id(t_queue * cola, char* id)
{
	//usar queue_size para sacar todos los elemento
	//menos el que necesito
}

clave_bloqueada_t * find_by_key(t_list * lista, char* key)
{

}