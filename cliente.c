#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef{
	int idNodo;
	int pid;
	int tipo;
}proceso;

//Tipos de procesos según prioridades
#define tipoPago 4
#define tipoAnulacion 3
#define tipoReserva 2
#define tipoGradaEvento 1



int main(int argc, char* argv[]){
	int argumento = atoi(arv[1]);

	switch(tipo){
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		default:
			printf("Argumento no válido\n1 para proceso Gradas\n2 para proceso Eventos\n");
			printf("3 para proceso Reservas\n4 para proceso Anulaciones\n5 para proceso Pagos\n");		
	}
	if(argumento<1&&argumento>5)
		return 0;

	
	
	
	return 0;
}