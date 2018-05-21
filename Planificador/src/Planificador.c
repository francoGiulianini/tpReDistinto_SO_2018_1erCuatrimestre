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
int listeningPort;
int socket_c;
char* ip_c;
char* port_c;
int fin_de_esi = 1;
t_esi* un_esi;
int clave_ok;
char* clave;
int respuesta_ok = 1;

int main(void)
{
	configure_logger();
    sem_init(&esi_listo, 1, 1);
    pthread_mutex_init(&pause_mutex, NULL);
    cola_ready = queue_create();

    config = config_create("Config.cfg");
    if(config == NULL)
        exit_with_error(logger, "Cannot open config file");

    get_values_from_config(logger, config);
	
    //aqui se conecta con el coordinador
    socket_c = connect_to_server(ip_c, port_c, "Coordinator");
    send_hello(socket_c);

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

    //sem_wait(&coordinador_listo);

	//Codigo del Planificador
	while (stop != 1)
	{    
		sem_wait(&esi_listo);
        pthread_mutex_lock(&pause_mutex);
        
        if(fin_de_esi)
            un_esi = queue_pop(cola_ready);
        
        send_message(un_esi->socket, 21); //ejecutar una instruccion

        wait_question(socket_c); //esperar pregunta del coordinador

        clave_ok = chequear_clave(clave);

        enviar_mensaje(socket_c, clave_ok); //31 clave libre 32 clave bloqueada

        esperar_respuesta_esi(un_esi->socket); //modifica la variable respuesta_ok
        
        //sem_wait(&coordinador_respuesta);
        if(!respuesta_ok)
        {
            bloquear_esi(un_esi);
        }

        pthread_mutex_unlock(&pause_mutex);
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

void HostConnections(/*void * parameter*/)
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
                        t_esi * new_esi = malloc(sizeof(t_esi));
                        num_esi++;

                        char* name = (char*)malloc(7);
                        if(num_esi < 10)
                            sprintf(name, "ESI0%d", num_esi);
                        else 
                            sprintf(name, "ESI%d", num_esi);

                        new_esi->socket = sd;
                        new_esi->name = name;
                        //memcpy(new_esi.name, name, strlen(name) + 1);  
                    
                        //se agrega a la cola de ready
                        queue_push(cola_ready, new_esi);
                        log_info(logger, "New ESI added to the queue as %s", new_esi->name);
                        
                        //free(name);
                        //free(new_esi->name);
                        //free(new_esi);
                    }

                    if(header->id == 22)
                    {
                        //ejecucion con exito
                    }
                    
                    if(header->id == 23)
                    {
                        //ejecucion bloquear
                    }

                    if(header->id == 24) //esi tiene mas instrucciones
                    {
                        fin_de_esi = 0;
                        sem_post(&esi_listo);
                    }

                    if(header->id == 25) //esi termino ejecucion
                    {
                        fin_de_esi = 1;
                        sem_post(&esi_listo);
                    }
                }  
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

void  send_hello(int socket) 
{
    content_header * header_c = (content_header*) malloc(sizeof(content_header));

    header_c->id=30;
    header_c->len=0;

	int result = send(socket, header_c, sizeof(content_header), 0);
	if (result <= 0)
		exit_with_error(logger, "Cannot send Hello");
    free(header_c);
}

void send_message(int socket, int id)
{
    send(socket, id, sizeof(id), 0);
}

void wait_question(int socket)
{
    int id;

    recv(socket, id, sizeof(id), 0);

    if (id == 32) //coordinador pregunta por clave bloqueada
    if (id == 33) //coordinador pide desbloquear clave
    if (id == 34) //coordinador no pregunta nada (operacion SET)
}