#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Declaramos las funciones que vamos a usar
void *coche(void *num);
void *camion(void *num);

// Declaramos las variables globales
pthread_mutex_t entrada; 	// mutex para controlar la entrada
pthread_mutex_t salida; 	// mutex para controlar la salida
pthread_cond_t espera; 		// condicion para soltar el mutex momentaneamente
int plazas; 			// plazas del parking
int * parking;			// array que representa el parking
int plazasLibres; 		// plazas libres del parking
int contCo; 			// contador para coches
int contCa; 			// contador para camiones
int contPlazas; 		// contador para plazas del parking

int main(int argc, char ** argv){

	//Declaramos las variables del main
	int plantas;
	int coches;
	int camiones;
	int *cochesId; 		// array para guardar los identificadores de los coches
	int *camionesId; 	// array para guardar los identificadores de los camiones
	pthread_t thr;

	// Control de errores
	if (argc < 3 || argc > 5) { // si introducimos argumentos de menos o de mas
		fprintf(stderr,"El número de argumentos introducidos es incorrecto\nUso: ./parking PLAZAS PLANTAS (COCHES CAMIONES)\n");
		exit(1); //salimos del programa
	} else { // tokenizamos la entrada en funcion del numero de argumentos
		if (argc == 3) {
			plazas = atoi (argv[1]);
			plantas = atoi (argv[2]);
			coches = 2 * plazas * plantas;
			camiones = 0;
		} else if (argc == 4) {
			plazas = atoi (argv[1]);
			plantas = atoi (argv[2]);
			coches = atoi (argv[3]);
			camiones = 0;
		} else {
			plazas = atoi (argv[1]);
			plantas = atoi (argv[2]);
			coches = atoi (argv[3]);
			camiones = atoi (argv[4]);
		}
		if (plazas < 1 || plantas < 1 || coches < 0 || camiones < 0 || (coches + camiones) < 1) {
			fprintf(stderr,"Valores no válidos, deben ser mayores que 1\n");
			exit(1); // si los valores son invalidos salimos del programa
		}
		if (plantas > 1) {
			fprintf(stderr,"Parking con más de una planta no implementado\n");
			exit(1); // salimos del programa
		}

		plazasLibres = plazas;

		// Reservamos memoria dinamica a partir de los datos tokenizados
		cochesId = (int *) malloc (coches * sizeof(int));
		camionesId = (int *) malloc (camiones * sizeof(int));
		parking = (int*) malloc(plazas * sizeof(int));

		// Inicializamos los mutex
		pthread_mutex_init(&entrada,NULL);
		pthread_mutex_init(&salida,NULL);

		// Creamos los hilos, uno por coche y uno por camión pasando el identificador por referencia
		for(contCo = 0; contCo < coches; contCo++) {
			cochesId[contCo] = contCo + 1; // los identificadores de los coches empiezan en 1
			pthread_create(&thr,NULL,coche,(void*)&cochesId[contCo]);
		}
		for(contCa = 0; contCa < camiones; contCa++) {
			camionesId[contCa] = contCa + 101; // los identificadores de los camiones empiezan en 101
			pthread_create(&thr,NULL,camion,(void*)&camionesId[contCa]);
		}

		while(1); // ejecutamos hasta que se interrumpa la ejecución del programa

	} // fin del else

} // fin del main

void *coche(void *num) { // funcion de los coches

	// Declaramos las variables locales
	int cocheId = *(int *)num;
	int i;
	int encontrado;

	while (1) { // el coche entra y sale, sin parar
		encontrado = -1; 					// mientras no se haya encontrado una plaza, vale -1
		sleep((rand() % 4) + 2); 				// esperamos un tiempo aleatorio

	// Entrando en sección crítica
		pthread_mutex_lock(&entrada); 				// cogemos el mutex para que nadie entre
		pthread_mutex_lock(&salida); 				// cogemos el mutex para que nadie salga

		// Djamos que salgan vehiculos hasta que haya sitio
		while(plazasLibres == 0)
			pthread_cond_wait(&espera, &salida);

		// Buscamos una plaza libre
		for (i = 0; i < plazas && encontrado == -1; i++) {
			if (parking[i] == 0)
				encontrado = i;
		}

		parking[encontrado] = cocheId; 				// ocupamos la plaza con el identificador del coche
		plazasLibres--; 					// disminuimos el numero de plazas
		printf("ENTRADA: Coche %d aparca en %d. Plazas libres: %d.\n",cocheId,encontrado,plazasLibres);

		// Mostramos el estado del parking
		printf("Parking: ");
		for (contPlazas = 0; contPlazas < plazas; contPlazas++)
			printf("[%d]\t ",parking[contPlazas]);
		printf("\n");

		pthread_mutex_unlock(&entrada); 			// soltamos el mutex, ya se puede entrar
		pthread_mutex_unlock(&salida); 				// soltamos el mutex, ya se puede salir
	// Saliendo de sección crítica

		sleep((rand() % 4) + 2);

	// Entrando en sección crítica
		pthread_mutex_lock(&salida); 				// cogemos el mutex, no se puede salir (ni entrar)
		parking[encontrado] = 0; 				// vaciamos la plaza ocupada
		plazasLibres++; 					// aumentamos el numero de plazas
		printf("SALIDA: Coche %d saliendo. Plazas libres: %d.\n",cocheId,plazasLibres);

		// Mostramos el estado del parking
		printf("Parking: ");
		for (contPlazas = 0; contPlazas < plazas; contPlazas++)
			printf("[%d]\t ",parking[contPlazas]);
		printf("\n");

		pthread_cond_signal(&espera); 				// avisamos que hemos salido por si habia alguien esperando
		pthread_mutex_unlock(&salida); 				// soltamos el mutex, ya se puede salir (y entrar)
	// Saliendo de sección crítica

	} // fin del while

} // fin de coche

void *camion(void *num) {

	// Declaramos las variables locales
	int camionId = *(int *)num;
	int i;
	int encontrado, encontrado2;

	while (1) { // el camion entra y sale, sin parar
		encontrado = -1;					// mientras no se haya encontrado una plaza, vale -1
		sleep((rand() % 4) + 2);				// esperamos un tiempo aleatorio

	// Entrando en sección crítica
		pthread_mutex_lock(&entrada);				// cogemos el mutex para que nadie entre
		pthread_mutex_lock(&salida);				// cogemos el mutex para que nadie salga

		// Dejamos salir a los vehiculos hasta que haya dos plazas libres
		while(plazasLibres < 2) {
			pthread_cond_wait(&espera, &salida);
		}

		// Dejamos salir a los vehiculos hasta que haya dos plazas libres consecutivas
		do {
			for (i = 0; i < (plazas - 1); i++) {
				if (parking[i] == 0 && parking[i+1] == 0) {
					encontrado = i;			// guardamos la primera plaza libre
					encontrado2 = i+1;		// guardamos la segunda plaza libre
				}
			}
			if (encontrado == -1)
				pthread_cond_wait(&espera, &salida); 	// si no podemos aparcar, dejamos salir otro vehiculo
		} while (encontrado == -1);

		// Ocupamos las dos plazas encontradas con el identificador del camion
		parking[encontrado] = camionId;
		parking[encontrado2] = camionId;

		plazasLibres = plazasLibres - 2;			// disminuimos el numero de plazas libres
		printf("ENTRADA: Camion %d aparca en %d y %d. Plazas libres: %d.\n", camionId, encontrado, encontrado2, plazasLibres);

		// Mostramos el estado del parking
		printf("Parking: ");
		for (contPlazas = 0; contPlazas < plazas; contPlazas++)
			printf("[%d]\t ",parking[contPlazas]);
		printf("\n");

		pthread_mutex_unlock(&entrada);				// soltamos el mutex, ya se puede entrar
		pthread_mutex_unlock(&salida);				// soltamos el mutex, ya se puede salir
	// Saliendo de sección crítica

		sleep((rand() % 4) + 2);

	// Entrando en sección crítica
		pthread_mutex_lock(&salida);				// cogemos el mutex, no se puede salir (ni entrar)

		// Vaciamos las plazas ocupadas
		parking[encontrado] = 0;
		parking[encontrado2] = 0;

		plazasLibres = plazasLibres + 2;			// aumentamos el numero de plazas
		printf("SALIDA: Camion %d saliendo. Plazas libres: %d.\n",camionId,plazasLibres);

		// Mostramos el estado del parking
		printf("Parking: ");
		for (contPlazas = 0; contPlazas < plazas; contPlazas++) {
			printf("[%d]\t ",parking[contPlazas]);
		}
		printf("\n");

		pthread_cond_signal(&espera);				// avisamos que hemos salido por si habia alguien esperando
		pthread_mutex_unlock(&salida);				// soltamos el mutex, ya se puede salir (y entrar)
	// Saliendo de sección crítica

	} // fin del while

} // fin del camion

