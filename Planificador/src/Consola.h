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

void *Console(void *parameter);
char * consoleReadArg(char * source, int * i);
_Commands to_enum(char * source);

#endif /* CONSOLA_H_ */