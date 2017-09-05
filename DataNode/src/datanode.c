#include "../../Biblioteca/src/configParser.c"

#define PARAMETROS {"IP_FILESYSTEM","PUERTO_FILESYSTEM","NOMBRE_NODO","PUERTO_WORKER","PUERTO_DATANODE","RUTA_DATABIN"}

int main(int argc, char **argv) {

	int codError; // Variable que se usa para absorber el codigo de error de una funcion
	char * pathConfiguracion; // Guarda el path de configuracion
	t_config * configuracion; // Estructura de configuracion
	char * parametros[] = PARAMETROS; // Guarda los parametros definidos en un array

	// Compruebo que no falten argumentos del main
	if(argc != 2){
		codError = -1;
		printf("Error con los parametros. \n"
				"Sintaxis: archivo.out 'path de archivo de configuracion' \n");
		return codError;
	}

	// Identifico y separo los argumentos del main
	pathConfiguracion = argv[1];

	// Compruebo existencia del archivo de configuracion en el path
	if(!existeArchivo(pathConfiguracion)){
		codError = -2;
		printf("El archivo de configuracion no existe. \n");
		return codError;
	}

	// Creo un puntero a archivo de configuracion
	configuracion = config_create(pathConfiguracion);

	//Imprimo los valores de la configuracion
	codError = imprimirConfiguracion(configuracion,parametros,sizeof(parametros)/sizeof(parametros[0]));
	switch (codError){
		case -3:
			printf("Error en la cantidad de parámetros en el archivo de configuración. \n");
			break;
		case -4:
			printf("Parametro definido en archivo de configuración no corresponde. \n");
			break;
	}

	return EXIT_SUCCESS;
}