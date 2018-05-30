# tp-2018-1c-Los-M-s-Mejores

* Protocolo ( a debatir)  : 

		- Header:  (Tipo + Tamaño De Clave + Tamaño de Valor)
		- Mensaje: (Clave + Valor)

* Como compilar:

 		-Planificador: (gcc -o Planificador Planificador.c Consola.c -lreadline -lcommons -pthread)
 		-Coordinador: (gcc -o Coordinador Coordinador.c -lcommons -pthread)

ID headers:(se le envian)(ID+TamañoMensaje)
 * -Coordinador:

  		-10: hola soy una instancia + tamaño del nombre
  		-11: instancia tiene que compactar
		-12: instancia guardo ok
		
  		-20: hola soy un esi
  		-21: esi pide un GET
  		-22: esi pide un SET
  		-23: esi pide un STORE
		
  		-30: hola soy el planificador
  		-31: clave esta libre
  		-32: clave no esta libre


 * -Planificador:

  		-20: hola soy un esi
  		-21: listo para ejecutar
  		-22: ejecucion con exito
  		-23: ejecucion bloqueo
		-24: termino ejecucion o aborto
  
  		-31: coordinador pide chequear clave  
  		-32: coordinador pide desbloquear clave
  		-33: coordinador no pide nada esi
		-34: coordinador pide abortar esi
		-35: coordinador pide iniciar planificacion

* -ESI:
		
		-21: planificador pide ejecutar una sentencia
		-22: coordinador dice que la clave esta bloqueada
		-23: coordinador dice que la clave no esta bloqueada
		-24: operacion exitosa(GET, SET o STORE)
		
* -Instancia:

		-11: coordinador pide compactar
		-12: coordinador pide SET
