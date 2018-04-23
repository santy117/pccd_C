#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>

//Constantes 
#define numMaxNodos 10
#define lectoresSC 4

//Tipos de procesos según prioridades
#define tipoPago 4
#define tipoAnulacion 3
#define tipoReserva 2
#define tipoGradaEvento 1

//Key para la cola de mensajes entre nodos
//Ningún nodo podra tener este ID
#define idInterNodo 600


//Estructuras definidas para la implementación
//Estructura utilizada por un proceso para pedir entrar en la SC
typedef struct{
	long type;
	int idNodo;
	int tipo;
	int pid;
}proceso;

//Estructura utilizada para pedir el testigo
typedef struct{
	long idNodoReceptor;
	int idNodoEmisor;
	int prioridad;
	int num;
}reqTestigo;

//Estructura utilizada para intercambiar el testigo
typedef struct{
	long idNodoReceptor;
	int vectorAtendidas[numMaxNodos+1];
	int vectorLeyendo[numMaxNodos+1];
}testigo;

typedef struct lista{
	int pid;
	int tipo;
	struct lista *siguiente;
} Lista;


//Semaforos
sem_t semaforoExclusionMutua;

//Variables intranodo
int idNodo;
int msqidNodo;
int msqidInterNodo;
int idOtrosNodos[numMaxNodos];
proceso peticiones[numMaxNodos+1];
proceso atendidas[numMaxNodos+1];
int token;
int dentro;

int numProcesos[4];
Lista *lista;



//Cabeceras funciones
void *receptorInterNodo(void);
void *receptorNodo(void);
void requestToken(void);
void sendToken(int);
void asignToken(void);
void addPetition(proceso);


int main(int argc, char* argv[]){
	idNodo = atoi(argv[1]);
	if(idNodo == idInterNodo){
		printf("ID reservado no se puede crear un nodo con este ID");
		exit(0);
	}
	else if(idNodo==1){
		token = 1;
	}
	else{
		token = 0;
	}

	char *path="/bin/ls";
	key_t keyNodo = ftok(path, idNodo);
	if(keyNodo == -1){
		printf("Error al crear la key del nodo\n");
		exit(0);
	}
	msqidNodo = msgget(keyNodo, IPC_CREAT|0666);
	if(msqidNodo==-1){
		printf("Error al crear cola nodo\n");
		exit(0);
	}

	key_t keyInterNodo = ftok(path, idInterNodo);
	if(keyInterNodo == -1){
		printf("Error al crear la key Internodo\n");
		exit(0);
	}
	msqidInterNodo = msgget(keyInterNodo, IPC_CREAT|0666);
	if(msqidInterNodo == -1){
		printf("Error al crear cola internodo\n");
		exit(0);
	}	

	sem_init(&semaforoExclusionMutua, 0, 1);
	
	pthread_t hiloReceptorInterNodo;
	pthread_t hiloReceptorNodo;
	pthread_create(&hiloReceptorInterNodo, NULL, receptorInterNodo, NULL);
	pthread_create(&hiloReceptorNodo, NULL, receptorNodo, NULL);

	while(1){
		printf("Nodo %i: reqTestigo=%i\n", idNodo, token);
		while(token==0);
		sem_wait(&semaforoExclusionMutua);
		sem_post(&semaforoExclusionMutua);
	}


	return 0;
}

void requestToken(){
	reqTestigo request;
	int i = 0, status;
	request.idNodoEmisor = idNodo;
	if(numProcesos[3]>0){
		request.prioridad = tipoPago;
		request.num = numProcesos[3];
	}
	else if(numProcesos[2]>0){
		request.prioridad = tipoAnulacion;
		request.num = numProcesos[2];
	}
	else if(numProcesos[1]>0){
		request.prioridad = tipoReserva;
		request.num = numProcesos[1];
	}
	else if(numProcesos[0]>0){
		request.prioridad = tipoGradaEvento;
		request.num = numProcesos[0];
	}
	else{
		request.prioridad = 0;
		request.num = 0;
	}
	for(i=0; i<numMaxNodos; i++){
		if(idOtrosNodos[i]==idNodo){
			continue;
		}
		else if(idOtrosNodos[i]==0){
			continue;
		}
		else{
			printf("Nodo %i: Solicitando testigo a nodo %i\n", idNodo, idOtrosNodos[i]);
			request.idNodoReceptor = idOtrosNodos[i];
			status = msgsnd(idInterNodo, &request, sizeof(reqTestigo), 0);
			if(status == -1){
				printf("Nodo %i: Error al pedir el testigo\n", idNodo);
				exit(0);
			}
		}
	}
}

void sendToken(int idNodoReceptor){
	sem_wait(&semaforoExclusionMutua);
	token = 0;
	sem_post(&semaforoExclusionMutua);
	testigo t;
	t.idNodoReceptor = idNodoReceptor;
	// **Falta código para enviar el vector de atendidas

	int status = msgsnd(idInterNodo, &t, sizeof(testigo), 0);
	if(status == -1){
		printf("Nodo %i: Error al enviar testigo\n", idNodo);
		exit(0);
	}

	//Función sin implementar comprueba si tenemos peticiones sin atender y pide el testigo
	//pendingMessages();
}

void asignToken(){

}

void *receptorNodo(){
	proceso p;
	int status;
	while(1){
		printf("Nodo %i: Esperando recibir procesos\n", idNodo);
		status = msgrcv(idNodo, &p, sizeof(proceso), 0, 0);
		if(status == -1){
			printf("Nodo %i: Error al recibir mensaje cola nodo\n", idNodo);
			exit(0);
		}
		sem_wait(&semaforoExclusionMutua);
		if(p.type == 2){
			printf("Nodo %i: Recibido mensaje del proceso %i de tipo %i para salida de SC", idNodo, p.pid, p.tipo);
		}
		else if(p.type == 1){
			printf("Nodo %i: Recibido mensaje del proceso %i de tipo %i para entrada de SC", idNodo, p.pid, p.tipo);
			addPetition(p);
		}else{
			msgsnd(idNodo, &p, sizeof(p), 0);
		}
		sem_post(&semaforoExclusionMutua);		

		addPetition(p);
	}
}

void addPetition(proceso p){
	printf("Nodo %i: añadida petición\n", idNodo);
	proceso pRespuesta;
	pRespuesta.type = p.pid;
	pRespuesta.pid = p.pid;
	int status = msgsnd(idNodo, &pRespuesta, sizeof(proceso), 0);
	if(status == -1){
		printf("Error\n");
		exit(0);
	}
}

void *receptorInterNodo(){
	reqTestigo t;
	int status;
	while(1){
		if(token==1)
			printf("Nodo %i: reqTestigo\n", idNodo);
		while(token==1);

		printf("Nodo %i: Esperando por el reqTestigo\n", idNodo);
		status = msgrcv(idInterNodo, &t, sizeof(reqTestigo), 0, 0);
		if(status == -1){
			printf("Nodo %i: Error al intentar recibir reqTestigo\n", idNodo);
			exit(0);
		}
		printf("Nodo %i: reqTestigo recibido\n", idNodo);

		sem_wait(&semaforoExclusionMutua);
		token = 1;
		sem_post(&semaforoExclusionMutua);
	}
}