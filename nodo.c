#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct{
	long tipo;
	int idNodo;
	int pid;
}proceso;

//Tipos de procesos según prioridades
#define tipoPago 4
#define tipoAnulacion 3
#define tipoReserva 2
#define tipoGradaEvento 1


//Variables intranodo
int[] idNodo;
proceso[] peticiones;
proceso[] atendidas;
int token;
int dentro;


//Cabeceras funciones
void receptor(void);
void requestToken(void);
void sendToken(int);
void asignToken(void);
void addPetition(proceso);


int main(int argc, char* argv[]){
	pthread_t hiloReceptor;

	return 0;
}

void requestToken(){

}

void sendToken(int idNodo){

}

void asignToken(){

}

void addPetition(proceso p){
	
}

void receptor(){

}