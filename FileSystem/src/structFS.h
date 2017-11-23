#include "../../Biblioteca/src/configParser.c"
#include "../../Biblioteca/src/Socket.c"
#include <commons/bitarray.h>
#include <sys/mman.h>

#ifndef STRUCTFS_H_
#define STRUCTFS_H_

// PROTOCOLO
#define REC_INFONODO 100
#define REC_BLOQUE 101
#define ESC_CORRECTA 102
#define ESC_INCORRECTA 103
#define ENV_LEER 104
#define ENV_ESCRIBIR 105
#define INFO_ARCHIVO_FS 13
#define ARCHIVO_NO_ENCONTRADO -14
#define DATOS_NODO 12
#define ALMACENADO_FINAL 15

#define PARAMETROS {"PUERTO_ESCUCHA"}

//------------------DIRECOTORIO------------------------
typedef struct t_directory {
	int index;
	char nombre[255];
	int padre;
} directorio;

//------------------TABLA ARCHIVOS------------------------
typedef struct
	__attribute__((packed)) {
	char* nombreArchivo;
	uint32_t tamanio;
	char* tipo;
	t_list* bloques;
} tablaArchivos;

typedef struct
	__attribute__((packed)) {
	char* nodo;
	uint32_t bloque;
} copia;

typedef struct
	__attribute__((packed)) {
	char* bloque;
	t_list* copias;
	uint32_t bytes;
} copiasXBloque;

//------------------TABLA NODOS------------------------
typedef struct
	__attribute__((packed)) {
	uint32_t tamanio;
	uint32_t libres;
	t_list* nodo;
	t_list* contenidoXNodo;
} tablaNodos;

typedef struct
	__attribute__((packed)) {
	char* nodo;
	uint32_t total;
	uint32_t libre;
	uint32_t porcentajeOcioso;
	uint32_t socket;
} contenidoNodo;

//------------------BITMAP------------------------
typedef struct
	__attribute__((packed)) {
	char* nodo;
	t_bitarray *bitarray;
} tablaBitmapXNodos;

//------------------DATOS CONEXION NODO------------------------
typedef struct
	__attribute__((packed)) {
	char* nodo;
	uint32_t puerto;
	char* ip;
} datosConexionNodo;

int PUERTO_ESCUCHA;
char * PATH_METADATA;

t_log* loggerFileSystem;
int hayNodos;
bool esEstadoSeguro;
tablaNodos* tablaGlobalNodos;
t_list* listaBitmap;
t_list* tablaGlobalArchivos;
bool hayEstadoAnterior;
t_list* listaConexionNodos;
t_list* listaConexionNodos;
t_list* listaDirectorios;

#endif /* STRUCTFS_H_ */
