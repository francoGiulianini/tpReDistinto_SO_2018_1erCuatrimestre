/*
 * Planificador.h
 *
 *  Created on: 19 abr. 2018
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <commons/log.h>
#include <commons/string.h>

typedef enum _Command {EXIT,PAUSE,RESUME,BLOCK,UNLOCK,LIST,KILL,STATUS,DEADLOCK,HELP,ERROR} _Commands;

void configure_logger();
void *Console(void *parameter);
char * consoleReadArg(char * source, int * i);
_Commands to_enum(char * source);

#endif /* PLANIFICADOR_H_ */
