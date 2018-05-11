# tp-2018-1c-Los-M-s-Mejores

* Protocolo ( a debatir)  : 

			- Header:  (Tipo + Tamaño Del Mensaje)
			- Mensaje: (Tamaño variable)

* Como compilar:

 -Planificador: gcc -o Planificador Planificador.c Consola.c -lreadline -lcommons -pthread
 
 -Coordinador: gcc -o Coordinador Coordinador.c -lcommons -pthread

ID headers:(se le envian)(ID+TamañoMensaje)
 * -Coordinador:

  -10: hola soy una instancia + tamaño del nombre
  
  -11:
  
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
  
  -31: coordinador pide chequear clave
  
  -32: coordinador pide desbloquear clave
 
