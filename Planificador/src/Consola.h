#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/log.h>
#include <commons/string.h>

typedef enum _Command {EXIT,PAUSE,RESUME,BLOCK,UNLOCK,LIST,KILL,STATUS,DEADLOCK,HELP,ERROR} _Commands;

int stop;
int blocked_esi_by_console;
pthread_mutex_t pause_mutex;
t_queue * cola_ready;
t_list * lista_bloqueados;

void Console();
char * consoleReadArg(char * source, int * i);
_Commands to_enum(char * source);
int null_argument(char* arg_console, char* for_logger);
void block_esi_by_console(char* key, char* id);
t_esi * find_by_id(t_queue * cola, char* id);
clave_bloqueada_t * find_by_key(t_list * lista, char* key);

#endif /* CONSOLA_H_ */