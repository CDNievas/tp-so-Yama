/*
 ============================================================================
 Name        : YAMA.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "reduccionLocal.h"
#include "transformacion.h"
#include "reduccionGlobal.h"
#include "funcionesYAMA.h"
#include "../../Biblioteca/src/Socket.c"
#include "../../Biblioteca/src/configParser.c"

void manejadorMaster(void* socketMasterCliente){
	int socketMaster = *(int*)socketMasterCliente;
	char* nombreArchivoPeticion = string_new();
	uint32_t nroMaster = obtenerNumeroDeMaster();
	log_info(loggerYAMA, "El numero de master del socket %d es %d.", socketMaster, nroMaster);
	t_list* listaDeCopias;
	char* nodoEncargado;
	while(1){ //USAR BOOLEAN PARA CORTAR CUANDO TERMINE LA OPERACION Y MATAR EL HILO
		int operacionMaster = recvDeNotificacion(socketMaster);
		switch (operacionMaster){
			case TRANSFORMACION:
				log_info(loggerYAMA, "Se recibio una peticion del socket %d para llevar a cabo una transformacion.", socketMaster);
				string_append(&nombreArchivoPeticion, recibirNombreArchivo(socketMaster)); //RECIBO TAMANIO DE NOMBRE Y NOMBRE
				solicitarArchivo(nombreArchivoPeticion);
				log_info(loggerYAMA, "Se envio la peticion del archivo a FileSystem.");
				t_list* listaDeBloquesDeArchivo = recibirInfoArchivo(); //RECIBO LOS DATOS DE LOS BLOQUES (CADA CHAR* CON SU LONGITUD ANTES)
				t_list* listaBalanceo = armarDatosBalanceo(listaDeBloquesDeArchivo);
				log_info(loggerYAMA, "Se recibieron los datos del archivo de FileSystem.");
				log_info(loggerYAMA, "Se prosigue a cargar la transformacion en la tabla de estados.");
				listaDeCopias = balancearTransformacion(listaDeBloquesDeArchivo, listaBalanceo);
				cargarTransformacion(socketMaster, nroMaster, listaDeBloquesDeArchivo, listaBalanceo);
				free(nombreArchivoPeticion);
				break;
			case TRANSFORMACION_TERMINADA:
				log_info(loggerYAMA, "Se recibio una peticion del master %d para finalizar una transformacion.", nroMaster);
				terminarTransformacion(nroMaster, socketMaster); //RECIBO TAMANIO NOMBRE NODO, NODO, NRO DE BLOQUE
				log_info(loggerYAMA, "Se termino la transformacion del master %d correctamente.", nroMaster);
				t_list* listaDelJob = obtenerListaDelNodo(nroMaster, socketMaster);
				if(sePuedeHacerReduccionLocal(listaDelJob)){
					log_info(loggerYAMA, "Se puede llevar a cabo la reduccion local.");
					cargarReduccionLocal(socketMaster, nroMaster, listaDelJob);
				}else{
					log_info(loggerYAMA, "No se puede llevar a cabo la reduccion local aun.");
				}
				list_destroy(listaDelJob);
				break;
			case REPLANIFICAR:
				//REPLANIFICAR, RECIBO DE MASTER EL NRO DE BLOQUE NOMBRE NODO.
				//A LA FUNCION REPLANIFICAR LE PASO EL MENSAJE Y EL NROMASTER
				break;
			case REDUCCION_LOCAL_TERMINADA:
				terminarReduccionLocal(nroMaster, socketMaster); //RECIBO NOMBRE NODO VA A HABER UNA UNICA INSTANCIA DE NODO HACIENDO REDUCCION LOCAL
				if(sePuedeHacerReduccionGlobal(nroMaster)){
					t_list* bloquesReducidos = filtrarReduccionesDelNodo(nroMaster);
					nodoEncargado = cargarReduccionGlobal(socketMaster, nroMaster, bloquesReducidos);
				}
				break;
			case REDUCCION_GLOBAL_TERMINADA:
				terminarReduccionGlobal(nroMaster);
				almacenadoFinal(socketMaster, nodoEncargado);
				break;
//			case FINALIZO:
//				reestablecerWL(listaDeCopias, nodoEncargado)
//				log_info(loggerYAMA, "El Master %d termino su Job.\nTerminando su ejecucioin.\nCerrando la conexion.", nroMaster);
//				pthread_detach(pthread_self());
//				break;
			case ERROR_REDUCCION_LOCAL:
				//ACTUALIZAR TABLA DE ESTADOS, ABORTAR REDUCCION LOCAL. FUNCION RECIBE NROMASTER
				log_error(loggerYAMA, "Error de reduccion local.\nAbortando el Job");
				sendDeNotificacion(socketMaster, ABORTAR);
				pthread_cancel(pthread_self());
				break;
			case ERROR_REDUCCION_GLOBAL:
				//ACTUALIZAR TABLA DE ESTADOS, ABORTAR REDUCCION GLOBAL. FUNCION RECIBE NROMASTER
				log_error(loggerYAMA, "Error de reduccion global.\nAbortando el Job");
				sendDeNotificacion(socketMaster, ABORTAR);
				pthread_cancel(pthread_self());
				break;
			case CORTO:
				log_error(loggerYAMA, "El master %d corto.", nroMaster);
				pthread_cancel(pthread_self());
				break;
			default:
				log_error(loggerYAMA, "La peticion recibida por el master %d es erronea.", socketMaster);
				pthread_cancel(pthread_self());
				break;
		}
	}
}

int main(int argc, char *argv[])
{
	signal(SIGUSR1, chequeameLaSignal);
	loggerYAMA = log_create("YAMA.log", "YAMA", 1, 0);
	chequearParametros(argc,2);
	t_config* configuracionYAMA = generarTConfig(argv[1], 6);
	//t_config* configuracionYAMA = generarTConfig("Debug/yama.ini", 6);
	cargarYAMA(configuracionYAMA);
	log_info(loggerYAMA, "Se cargo exitosamente YAMA.");
	nodosSistema = list_create();
	socketFS = conectarAServer(FS_IP, FS_PUERTO);
	handshakeFS();
	int socketEscuchaMasters = ponerseAEscucharClientes(PUERTO_MASTERS, 0);
 	log_info(loggerYAMA, "Escuchando clientes...");
	int socketMaximo = socketEscuchaMasters, socketClienteChequeado, socketAceptado;
	fd_set socketsMasterCPeticion, socketMastersAuxiliares;
	FD_ZERO(&socketMastersAuxiliares);
	FD_ZERO(&socketsMasterCPeticion);
	FD_SET(socketEscuchaMasters, &socketsMasterCPeticion);
 	pthread_t hiloManejadorMaster;
 	pthread_attr_t attr;
 	pthread_attr_init(&attr);
 	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  	tablaDeEstados = list_create();
	while(1){
		socketMastersAuxiliares = socketsMasterCPeticion;
		if(select(socketMaximo+1, &socketMastersAuxiliares, NULL, NULL, NULL)==-1){
			log_error(loggerYAMA, "No se pudo llevar a cabo el select en YAMA.");
			exit(-1);
		}
		log_info(loggerYAMA, "Un socket realizo una peticion a YAMA.");
		for(socketClienteChequeado = 0; socketClienteChequeado <= socketMaximo; socketClienteChequeado++){
			if(FD_ISSET(socketClienteChequeado, &socketMastersAuxiliares)){
				if(socketClienteChequeado == socketEscuchaMasters){
					socketAceptado = aceptarConexionDeCliente(socketEscuchaMasters);
					FD_SET(socketAceptado, &socketsMasterCPeticion);
					socketMaximo = calcularSocketMaximo(socketAceptado, socketMaximo);
					log_info(loggerYAMA, "Se recibio una nueva conexion del socket %d.", socketAceptado);
				}else{
					int notificacion = recvDeNotificacion(socketClienteChequeado);
					if(notificacion != ES_MASTER){
						log_error(loggerYAMA, "La conexion del socket %d es erronea.", socketClienteChequeado);
						FD_CLR(socketClienteChequeado, &socketsMasterCPeticion);
						close(socketClienteChequeado);
					}else{
						sendDeNotificacion(socketClienteChequeado, ES_YAMA);
           				log_info(loggerYAMA, "Se establecio la conexion con el socket master %d. \n Handshake realizado.", socketClienteChequeado);
            			int* socketCliente = malloc(sizeof(int));
            			*socketCliente = socketClienteChequeado;
            			pthread_create(&hiloManejadorMaster, &attr, (void*)manejadorMaster, (void*)socketCliente);
            			log_info(loggerYAMA, "Pasando a atender la peticion del socket master %d.", socketClienteChequeado);
           				FD_CLR(socketClienteChequeado, &socketsMasterCPeticion);
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}