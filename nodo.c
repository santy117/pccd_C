#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/time.h>

//Constantes 
#define numMaxNodos 4
#define lectoresSC 4

//Tipos de procesos según prioridades
#define tipoPago 4
#define tipoAnulacion 3
#define tipoReserva 2
#define tipoGradaEvento 1

//Key para la cola de mensajes entre nodos
//Ningún nodo podra tener este ID
#define idInterNodoTestigo 600
#define idInterNodoRequest 601

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
	int idNodoEmisor;
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
int msqidInterNodoTestigo;
int msqidInterNodoRequest;
int idOtrosNodos[numMaxNodos+1] = {0, 1, 2, 3, 4};
int atendidas[numMaxNodos+1] = {0, 0, 0, 0, 0};
int leyendo[numMaxNodos+1] = {0, 0, 0, 0, 0};
int prioridades[numMaxNodos+1] = {0, 0, 0, 0, 0};
int token = 0;
int dentro = 0;
int myNum = 0;
int myPrio = 0;
int leyendoBool = 0;

int numProcesos[5] = {0, 0, 0, 0, 0};
Lista *lista[5];


//Cabeceras funciones
void *receptorInterNodo(void);
void *receptorNodo(void);
void requestToken(void);
void sendToken(int);
void asignToken(void);
void addPetition(proceso);
long long int getTimestamp(void);

int main(int argc, char* argv[]){
	int i, status;
	for(i=0; i<5; i++){
		lista[i] = (Lista *)malloc(sizeof(Lista));
	}
	idNodo = atoi(argv[1]);
	if(idNodo == idInterNodoTestigo || idNodo == idInterNodoRequest){
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

	key_t keyInterNodo = ftok(path, idInterNodoTestigo);
	if(keyInterNodo == -1){
		printf("Error al crear la key Internodo\n");
		exit(0);
	}
	msqidInterNodoTestigo = msgget(keyInterNodo, IPC_CREAT|0666);
	if(msqidInterNodoTestigo == -1){
		printf("Error al crear cola internodo testigo\n");
		exit(0);
	}	

	key_t keyInterNodoRequest =  ftok(path, idInterNodoRequest);
	if(keyInterNodoRequest == -1){
		printf("Nodo %i: Error al crear key internodo peticiones", idNodo);
		exit(0);
	}
	msqidInterNodoRequest = msgget(keyInterNodoRequest, IPC_CREAT|0666);
	if(msqidInterNodoRequest == -1){
		printf("Nodo %i: Error al crear cola internodo peticiones", idNodo);
		exit(0);
	}

	sem_init(&semaforoExclusionMutua, 0, 1);

	pthread_t hiloReceptorInterNodo;
	pthread_t hiloReceptorNodo;
	pthread_create(&hiloReceptorInterNodo, NULL, receptorInterNodo, NULL);
	pthread_create(&hiloReceptorNodo, NULL, receptorNodo, NULL);

	while(1){
		printf("Nodo %i [%lld]: Testigo=%i\n", idNodo, getTimestamp(), token);
		printf("Nodo %i [%lld]: Prioridad=%i\n", idNodo, getTimestamp(), myPrio);
		reqTestigo request;
		status = msgrcv(msqidInterNodoRequest, &request, sizeof(reqTestigo), idNodo, 0);
		if(status == -1){
			printf("Nodo %i: Error al recibir petición de testigo\n", idNodo);
			printf("%i", errno);
			exit(0);
		}
		sem_wait(&semaforoExclusionMutua);
		printf("Nodo %i [%lld]: Petición de nodo %i de testigo Prioridad=%i\n", idNodo, getTimestamp(), request.idNodoEmisor, request.prioridad);
		if(request.prioridad > prioridades[request.prioridad])
			prioridades[request.idNodoEmisor] = request.prioridad;
		atendidas[request.idNodoEmisor] = request.num;
		if(token == 1 && dentro == 0)
			asignToken();
		sem_post(&semaforoExclusionMutua);
	}

	return 0;
}

void requestToken(){
	if(token==1)
		return;
	reqTestigo request;
	int i = 0, status;
	request.idNodoEmisor = idNodo;
	request.num = myNum;
	if(numProcesos[tipoPago]>0){
		request.prioridad = tipoPago;
		myPrio = tipoPago;
	}
	else if(numProcesos[tipoAnulacion]>0){
		request.prioridad = tipoAnulacion;
		myPrio = tipoAnulacion;
	}
	else if(numProcesos[tipoReserva]>0){
		request.prioridad = tipoReserva;
		myPrio = tipoReserva;
	}
	else if(numProcesos[tipoGradaEvento]>0){
		request.prioridad = tipoGradaEvento;
		myPrio = tipoGradaEvento;
	}
	else{
		request.prioridad = 0;
		return;
	}
	for(i=0; i<numMaxNodos+1; i++){
		if(idOtrosNodos[i]==idNodo){
			continue;
		}
		else if(idOtrosNodos[i]==0){
			continue;
		}
		else{
			printf("Nodo %i [%lld]: Solicitando testigo a nodo %i con prioridad %i\n", idNodo, getTimestamp(), idOtrosNodos[i], request.prioridad);
			request.idNodoReceptor = idOtrosNodos[i];
			status = msgsnd(msqidInterNodoRequest, &request, sizeof(reqTestigo), 0);
			if(status == -1){
				printf("Nodo %i: Error al pedir el testigo\n", idNodo);
				exit(0);
			}
		}
	}
}

void sendToken(int idNodoReceptor){
	token = 0;
	testigo t;
	t.idNodoReceptor = idNodoReceptor;
	t.idNodoEmisor = idNodo;
	atendidas[idNodo] = myNum;
	memcpy(t.vectorAtendidas, atendidas, sizeof(atendidas));
	memcpy(t.vectorLeyendo, leyendo, sizeof(leyendo));
	printf("Nodo %i [%lld]: Enviando testigo a nodo %i\n", idNodo, getTimestamp(), idNodoReceptor);
	int status = msgsnd(msqidInterNodoTestigo, &t, sizeof(testigo), 0);
	if(status == -1){
		printf("Nodo %i: Error al enviar testigo\n", idNodo);
		exit(0);
	}
	myPrio = 0;
	if(numProcesos[tipoPago]>0||numProcesos[tipoAnulacion]>0||numProcesos[tipoReserva]>0||numProcesos[tipoGradaEvento]>0)
		requestToken();
}

void asignToken(){
	int status, i, lectores;
	if(token == 0 || dentro == 1)
		return;
	
	if(numProcesos[tipoPago]>0){
		if(leyendoBool==1){
			return;
		}
		printf("Nodo %i [%lld]: Enviando permiso SC a proceso %i\n", idNodo, getTimestamp(), lista[tipoPago]->pid);
		proceso p;
		p.type = lista[tipoPago]->pid;
		p.pid = lista[tipoPago]->pid;
		status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
		if(status == -1){
			printf("Nodo %i: Error al enviar mensaje a proceso %i\n", idNodo, lista[tipoPago]->pid);
			exit(0);
		}
		dentro = 1;
		Lista *aux=lista[tipoPago];
		lista[tipoPago] = lista[tipoPago]->siguiente;
		free(aux);
		numProcesos[tipoPago]--;
		if(numProcesos[tipoPago]==0)
			lista[tipoPago] = (Lista *)malloc(sizeof(Lista));
		else
			myPrio = tipoPago;
		return;
	}
	else{
		if(leyendo[idNodo] == 0){
			int otro = 0;
			for(int i=0; i<numMaxNodos+1; i++){
				if(prioridades[i]==tipoPago && i!= idNodo){
					otro = i;
				}
			}
			if(otro != 0){
				printf("Nodo %i [%lld]: Nodo %i tiene mayor prioridad\n", idNodo, getTimestamp(), otro);
				sendToken(otro);
				return;
			}
		}
	}

	if(numProcesos[tipoAnulacion]>0){
		if(leyendoBool==1){
			return;
		}
		printf("Nodo %i [%lld]: Enviando permiso SC a proceso %i\n", idNodo, getTimestamp(),lista[tipoAnulacion]->pid);
		proceso p;
		p.type = lista[tipoAnulacion]->pid;
		p.pid = lista[tipoAnulacion]->pid;
		status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
		if(status == -1){
			printf("Nodo %i: Error al enviar mensaje a proceso %i\n", idNodo, lista[tipoAnulacion]->pid);
			exit(0);
		}
		dentro = 1;
		Lista *aux = lista[tipoAnulacion];
		lista[tipoAnulacion] = lista[tipoAnulacion]->siguiente;
		free(aux);
		numProcesos[tipoAnulacion]--;
		if(numProcesos[tipoAnulacion]==0)
			lista[tipoAnulacion] = (Lista *)malloc(sizeof(Lista));
		else
			myPrio =tipoAnulacion;
		return;
	}
	else{
		if(leyendo[idNodo] == 0){	
			int otro = 0;
			for(int i=0; i<numMaxNodos+1; i++){
				if(prioridades[i]==tipoAnulacion && i!= idNodo){
					otro = i;
				}
			}
			
			if(otro != 0){
				printf("Nodo %i [%lld]: Nodo %i tiene mayor prioridad\n", idNodo, getTimestamp(), otro);
				sendToken(otro);
				return;
			}
		}
	}

	if(numProcesos[tipoReserva]>0){
		if(leyendoBool==1){
			return;
		}
		printf("Nodo %i [%lld]: Enviando permiso SC a proceso %i\n", idNodo, getTimestamp(), lista[tipoReserva]->pid);
		proceso p;
		p.type = lista[tipoReserva]->pid;
		p.pid = lista[tipoReserva]->pid;
		status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
		if(status == -1){
			printf("Nodo %i: Error al enviar mensaje al proceso %i\n", idNodo, lista[tipoReserva]->pid);
			exit(0);
		}
		dentro = 1;
		Lista *aux = lista[tipoReserva];
		lista[tipoReserva] = lista[tipoReserva]->siguiente;
		free(aux);
		numProcesos[tipoReserva]--;
		if(numProcesos[tipoReserva]==0)
			lista[tipoReserva] = (Lista *)malloc(sizeof(Lista));
		else 
			myPrio = tipoReserva;
		return;
	}
	else{
		if(leyendo[idNodo] == 0){
			int otro = 0;
			for(int i=0; i<numMaxNodos+1; i++){
				if(prioridades[i]==tipoReserva && i!= idNodo){
					otro = i;
				}
			}
			if(otro != 0){
				printf("Nodo %i [%lld]: Nodo %i tiene mayor prioridad\n", idNodo, getTimestamp(), otro);
				sendToken(otro);
				return;
			}
		}
	}

	if(numProcesos[tipoGradaEvento]>0){
		if(dentro == 1)
			return;
		if(numProcesos[tipoPago]>0 || numProcesos[tipoAnulacion]>0 || numProcesos[tipoReserva]>0){
			//Esto significa que hay otro proceso más prioritario en el nodo por lo que no se atenderan otro lector
			return;
		}
		status = 0;
		for(i=0; i<numMaxNodos+1; i++){
			if(prioridades[i]>tipoGradaEvento){
				status = 1;
			}
		}
		if(status == 1)
			return;
		lectores = 0;
		for(i=0; i<numMaxNodos+1; i++){
			lectores += leyendo[i];
		}
		if(lectores>lectoresSC){
			//Esto significa que tenemos N lectores concurrentes y no podemos tener más
			printf("Nodo %i [%lld]: Hay N lectores en SC\n", idNodo, getTimestamp());
			return;
		}
		printf("Nodo %i [%lld]: Lectores SC %i\n", idNodo, getTimestamp(), lectores);
		printf("Nodo %i [%lld]: Enviando permiso SC a proceso %i\n", idNodo, getTimestamp(),lista[tipoGradaEvento]->pid);
		proceso p;
		p.type = lista[tipoGradaEvento]->pid;
		p.pid = lista[tipoGradaEvento]->pid;
		status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
		if(status == -1){
			printf("Nodo %i: Error al enviar mensaje al proceso %i\n", idNodo, lista[tipoGradaEvento]->pid);
			exit(0);
		}
		//Lista *aux = lista[tipoReserva];
		lista[tipoGradaEvento] = lista[tipoGradaEvento]->siguiente;
		//free(aux);
		leyendoBool = 1;
		leyendo[idNodo]++;
		numProcesos[tipoGradaEvento]--;
		if(numProcesos[tipoGradaEvento]==0)
			lista[tipoGradaEvento] = (Lista *)malloc(sizeof(Lista));
		else{
			myPrio = tipoGradaEvento;
			asignToken();
		}
	}
	else{
		if(dentro == 1)
			return;
		int otro = 0;
		for(int i=0; i<numMaxNodos+1; i++){
			if(prioridades[i]==tipoGradaEvento && i!= idNodo){
				otro = i;
			}
		}
		if(otro != 0){
			printf("Nodo %i [%lld]: Nodo %i tiene lectores esperando\n", idNodo, getTimestamp(), otro);
			sendToken(otro);
			return;
		}
		
	}

}

void *receptorNodo(){
	proceso p;
	int status,i, lectores;
	while(1){
		//printf("Nodo %i: Esperando recibir procesos\n", idNodo);
		status = msgrcv(msqidNodo, &p, sizeof(proceso), 0, 0);
		if(status == -1){
			printf("Nodo %i: Error al recibir mensaje cola nodo\n", idNodo);
			exit(0);
		}
		sem_wait(&semaforoExclusionMutua);
		if(p.type == 2){
			printf("Nodo %i [%lld]: Recibido mensaje del proceso %i de tipo %i para salida de SC\n", idNodo, getTimestamp(), p.pid, p.tipo);
			if(p.tipo>1)
				dentro = 0;
			myNum++;
			if(p.tipo==tipoGradaEvento){
				leyendo[idNodo]--;
				lectores = 0;
				for(i=0; i<numMaxNodos; i++){
					lectores += leyendo[i];
				}
				if(lectores==0){
					leyendoBool = 0;
				}
			}
			if(numProcesos[tipoPago]==0)
				myPrio = tipoAnulacion;
			if(numProcesos[tipoAnulacion]==0)
				myPrio = tipoReserva;
			if(numProcesos[tipoReserva]==0)
				myPrio = tipoGradaEvento;
			if(numProcesos[tipoGradaEvento]==0)
				myPrio = 0;
			asignToken();
		}
		else if(p.type == 1){
			printf("Nodo %i [%lld]: Recibido mensaje del proceso %i de tipo %i para entrada de SC\n", idNodo, getTimestamp(), p.pid, p.tipo);
			addPetition(p);
			asignToken();
		}else{
			msgsnd(msqidNodo, &p, sizeof(p), 0);
		}
		sem_post(&semaforoExclusionMutua);		
	}
}

void addPetition(proceso p){
	printf("Nodo %i [%lld]: Recibe peticion\n", idNodo, getTimestamp());
	printf("\t\t\t   Proceso: %i\n", p.pid);
	printf("\t\t\t   Tipo: %i\n", p.tipo);
	
	Lista *aux = (Lista *)malloc(sizeof(Lista));
	aux->pid = p.pid;
	aux->tipo = p.tipo;
	aux->siguiente = NULL;
	numProcesos[p.tipo]++;
	printf("Nodo %i [%lld]: Procesos tipo %i en el nodo: %i\n", idNodo, getTimestamp(), p.tipo, numProcesos[p.tipo]);
	if(numProcesos[p.tipo]==1){
		lista[p.tipo]->pid = p.pid;
		lista[p.tipo]->tipo = p.tipo;
		lista[p.tipo]->siguiente = NULL;
	}
	else{
		Lista *l;
		l = lista[p.tipo];
		while(l->siguiente!=NULL){
			l = l->siguiente;
		}
		l->siguiente = aux;
	}
	if(p.tipo>myPrio && token != 1)
		requestToken();
}

void *receptorInterNodo(){
	testigo t;
	int status;
	while(1){
		if(token==1)
			printf("Nodo %i [%lld]: Testigo=1\n", idNodo, getTimestamp());
		while(token==1);

		//printf("Nodo %i: Esperando por el Testigo\n", idNodo);
		status = msgrcv(msqidInterNodoTestigo, &t, sizeof(testigo), idNodo, 0);
		if(status == -1){
			printf("Nodo %i: Error al intentar recibir Testigo\n", idNodo);
			exit(0);
		}
		sem_wait(&semaforoExclusionMutua);
		printf("Nodo %i [%lld]: Testigo recibido\n", idNodo, getTimestamp());
		
		token = 1;
		prioridades[t.idNodoEmisor] = 0;
		memcpy(atendidas, t.vectorAtendidas, sizeof(atendidas));
		int lecAux = leyendo[idNodo];
		memcpy(leyendo, t.vectorLeyendo, sizeof(leyendo));
		atendidas[idNodo] = myNum;
		leyendo[idNodo] = lecAux;
		printf("VECTOR ATENDIDAS\n");
		for(status = 1; status < numMaxNodos+1; status++){
			printf("ATENDIDAS NODO %i -> %i\n", status, atendidas[status]);
		}
		printf("VECTOR LEYENDO\n");
		for(status = 1; status < numMaxNodos+1; status++){
			printf("LEYENDO NODO %i -> %i\n", status, leyendo[status]);
		}
		int aux = 0, nodo = 0;
		for(status=0; status < numMaxNodos+1; status++){
			aux += leyendo[status];
			if(leyendo[status]!= 0){
				printf("%i\n", status);
				nodo = status;
			}
		}
		if(aux != 0)
			leyendoBool = 1;

		printf("Lectores en nodo %i\n", leyendo[idNodo]);
		printf("LeyendoBool %i\n", leyendoBool);
		if(leyendo[idNodo]==0){
			if(leyendoBool == 1){
				sendToken(nodo);
				requestToken();
				sem_post(&semaforoExclusionMutua);
			}
			else{
				asignToken();
				sem_post(&semaforoExclusionMutua);
			}
		}
		else{
			asignToken();
			sem_post(&semaforoExclusionMutua);
		}
	}
}


long long int getTimestamp(){
	struct timeval timer_usec; 
  	long long int timestamp_usec;
	if (!gettimeofday(&timer_usec, NULL)) {
    	timestamp_usec = ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec;
  	}
  	else {
    	timestamp_usec = -1;
  	}
}