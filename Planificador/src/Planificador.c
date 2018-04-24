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

pthread_t idConsole;
pthread_t idHostConnections;
t_log * logger;
struct sockaddr_in serverAddress;
int consoleInputIndex = 0;
int stop = 0;

int main(void)
{
	configure_logger();

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY; //leer de archivo de config
	serverAddress.sin_port = htons(8080);

	int error = pthread_create(&idConsole, NULL, Console, NULL);

	if(error != 0)
	{
		//Logear que no se creo el hilo
		log_error(logger, "Couldn't create Console");
	}

	//aca se tendria que conectar con los demas modulos
	error = pthread_create(&idHostConnections, NULL, HostConnections, NULL);

	if(error != 0)
	{
		//Logear que no se creo el hilo
		log_error(logger, "Couldn't create Server Thread");
	}
	
	//Codigo del Planificador
	while (stop != 1)
	{
		
	}

	return EXIT_SUCCESS;
}

void configure_logger()
{
  logger = log_create("Planificador.log", "Planificador", true, LOG_LEVEL_INFO);
}

void *Console(void *parameter)
{
	int exit = 0; //flag de hilo consola
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
				log_info(logger, "The Scheduler was paused by command");
				break;
			}
			case RESUME:
			{
				//el panificador vuelve de la pausa
				log_info(logger, "The Scheduler was unpaused by command");
				break;
			}
			case BLOCK:
			{
				//bloquear ESI
				key = consoleReadArg(line, &consoleInputIndex);
				id = consoleReadArg(line, &consoleInputIndex);
				log_info(logger, "The Scheduler blocked an ESI with key: %s and id: %s by command", key, id);
				free(key);
				free(id);
				break;
			}
			case UNLOCK:
			{
				//desbloquear clave
				key = consoleReadArg(line, &consoleInputIndex);
				log_info(logger, "The Scheduler unlocked the key: %s by command", key);
				free(key);
				break;
			}
			case LIST:
			{
				//mostrar los esi bloqueados por el recurso
				resource = consoleReadArg(line, &consoleInputIndex);
				log_info(logger, "List of ESIs blocked waiting for resource: %s by command", resource);
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
				printf("exit\npause\nresume\nblock\nunlock\nlist\nkill\nstatus\ndeadlock\n");
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

void *HostConnections(void * parameter)
{
	char * message = "Welcome";
	int opt = 1;
	int master_socket, new_socket , client_socket[30] , 
          max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
	int addrlen = sizeof(serverAddress);
	char * buffer;
	fd_set read_fds;

	for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }

	master_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (master_socket < 0)
	{
		log_error(logger, "Failed to create Socket");
		exit(-1);
	}

	int activated = 1;

	setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &activated, sizeof(activated));

	if (bind(master_socket, (void *)&serverAddress, sizeof(serverAddress)) != 0)
	{
		log_error(logger, "Failed to bind");
		exit(-1);
	}

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
				"New connection , socket fd is %d , ip is : %s , port : %d \n", 
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
                    log_info(logger, "Adding to list of sockets as %d\n" , i);  
                       
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
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, 1024)) == 0)  //TIENE QUE SEGUIR EL PROTOCOLO
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
                   
                //Echo back the message that came in 
                else 
                {  
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    buffer[valread] = '\0';  
                    send(sd , buffer , strlen(buffer) , 0 );  
                }  
            }  
        }
	}
}