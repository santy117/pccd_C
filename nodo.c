#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
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
sem_t semaforoExclusionMutuaDentro;

//Variables intranodo
int idNodo;
int msqidNodo;
int msqidInterNodo;
int idOtrosNodos[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int peticiones[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int atendidas[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int leyendo[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int prioridades[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int token = 0;
int dentro = 0;
int myNum = 0;
int leyendoBool = 0;

int numProcesos[5] = {0, 0, 0, 0, 0};
Lista *lista[5];


//Cabeceras funciones
void *receptorInterNodo(void);
void *receptorNodo(void);
void requestToken(void);
void sendToken(int);
void *asignToken(void);
void addPetition(proceso);
long long int getTimestamp(void);

int main(int argc, char* argv[]){
	int i, status;
	for(i=0; i<sizeof(leyendo); i++)
		leyendo[i]=0;
	for(i=0; i<5; i++){
		lista[i] = (Lista *)malloc(sizeof(Lista));
	}
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
	sem_init(&semaforoExclusionMutuaDentro, 0, 1);

	pthread_t hiloReceptorInterNodo;
	pthread_t hiloReceptorNodo;
	pthread_t hiloAsignaToken;
	pthread_create(&hiloReceptorInterNodo, NULL, receptorInterNodo, NULL);
	pthread_create(&hiloReceptorNodo, NULL, receptorNodo, NULL);
	pthread_create(&hiloAsignaToken, NULL, asignToken, NULL);

	while(1){
		printf("Nodo %i: Testigo=%i\n", idNodo, token);
		while(token==0);
		//printf("Nodo %i: Esperando a recibir peticiones del testigo\n", idNodo);
		reqTestigo request;
		status = msgrcv(msqidInterNodo, &request, sizeof(reqTestigo), idNodo, 0);
		if(status == -1){
			printf("Nodo %i: Error al recibir petición de tesigo\n", idNodo);
			exit(0);
		}
		sem_wait(&semaforoExclusionMutua);
		printf("Nodo %i: Petición enviada por %i\n", idNodo, request.idNodoEmisor);
		printf("\t\tPrioridad de petición %i\n", request.prioridad);
		prioridades[request.idNodoEmisor] = request.prioridad;
		atendidas[request.idNodoEmisor] = request.num;
		if(request.prioridad==tipoPago && numProcesos[tipoPago]==0){
			sendToken(request.idNodoEmisor);
		}
		else if(request.prioridad==tipoAnulacion && numProcesos[tipoAnulacion]==0){
			int envio = 0;
			for(i=0; i<sizeof(prioridades); i++){
				if(prioridades[i]==tipoPago){
					envio = i;
				}
			}
			if(envio!=0){
				sendToken(idOtrosNodos[i]);
			}
			else{
				sendToken(request.idNodoEmisor);
			}
		}
		else if(request.prioridad==tipoReserva && numProcesos[tipoReserva]==0){
			int envio = 0;
			for(i=0; i<sizeof(prioridades); i++){
				if(prioridades[i]==tipoAnulacion){
					envio = i;
				}
			}
			if(envio != 0){
				sendToken(idOtrosNodos[i]);
			}
			else{
				sendToken(request.idNodoEmisor);
			}
		}
		else if(request.prioridad==tipoGradaEvento && numProcesos[tipoGradaEvento]==0){
			int envio = 0;
			for(i=0; i<sizeof(prioridades); i++){
				if(prioridades[i]==tipoReserva){
					envio = i;
				}
			}
			if(envio != 0){
				sendToken(idOtrosNodos[i]);
			}
			else{
				sendToken(request.idNodoEmisor);
			}
		}
		sem_post(&semaforoExclusionMutua);
	}

	return 0;
}

void requestToken(){
	reqTestigo request;
	int i = 0, status;
	request.idNodoEmisor = idNodo;
	request.num = myNum;
	if(numProcesos[tipoPago]>0){
		request.prioridad = tipoPago;
	}
	else if(numProcesos[tipoAnulacion]>0){
		request.prioridad = tipoAnulacion;
	}
	else if(numProcesos[tipoReserva]>0){
		request.prioridad = tipoReserva;
	}
	else if(numProcesos[tipoGradaEvento]>0){
		request.prioridad = tipoGradaEvento;
	}
	else{
		request.prioridad = 0;
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
			status = msgsnd(msqidInterNodo, &request, sizeof(reqTestigo), 0);
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
	memcpy(t.vectorAtendidas, atendidas, sizeof(atendidas));
	memcpy(t.vectorLeyendo, leyendo, sizeof(leyendo));

	int status = msgsnd(msqidInterNodo, &t, sizeof(testigo), 0);
	if(status == -1){
		printf("Nodo %i: Error al enviar testigo\n", idNodo);
		exit(0);
	}
	//Función sin implementar comprueba si tenemos peticiones sin atender y pide el testigo
	//pendingMessages();
}

void *asignToken(){
	int status, i, lectores;
	while(1){
		while(token==0);
		sem_wait(&semaforoExclusionMutua);
		if(numProcesos[tipoPago]>0){
			if(dentro==1||leyendoBool==1){
				sem_post(&semaforoExclusionMutua);
				continue;
			}
			printf("Nodo %i: Enviando permiso SC a proceso %i\n", idNodo, lista[tipoPago]->pid);
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
		}
		if(numProcesos[tipoAnulacion]>0){
			if(dentro==1 || leyendoBool==1){
				sem_post(&semaforoExclusionMutua);
				continue;
			}
			printf("Nodo %i: Enviando permiso SC a proceso %i\n", idNodo, lista[tipoAnulacion]->pid);
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
		}
		if(numProcesos[tipoReserva]>0){
			if(dentro==1||leyendoBool==1){
				sem_post(&semaforoExclusionMutua);
				continue;
			}
			printf("Nodo %i: Enviando permiso SC a proceso %i\n", idNodo, lista[tipoReserva]->pid);
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
			if(lista[tipoReserva]==0)
				lista[tipoReserva] = (Lista *)malloc(sizeof(Lista));
		}
		if(numProcesos[tipoGradaEvento]>0){
			if(dentro==1){
				sem_post(&semaforoExclusionMutua);
				continue;
			}
			if(numProcesos[tipoPago]>0||numProcesos[tipoAnulacion]>0||numProcesos[tipoReserva]>0){
				//Esto significa que hay otro proceso más prioritario en el nodo por lo que no se atenderan otro lector
				sem_post(&semaforoExclusionMutua);
				continue;
			}
			lectores = 0;
			for(i=0; i<numMaxNodos+1; i++){
				lectores += leyendo[i];
			}
			if(lectores>=lectoresSC){
				//Esto significa que tenemos N lectores concurrentes y no podemos tener más
				printf("Nodo %i: Hay N lectores en SC\n", idNodo);
				sem_post(&semaforoExclusionMutua);
				continue;
			}
			printf("Lectores SC %i\n", lectores);
			printf("Nodo %i: Enviando permiso SC a proceso %i\n", idNodo, lista[tipoGradaEvento]->pid);
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
		}
		sem_post(&semaforoExclusionMutua);
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
			printf("Nodo %i: Recibido mensaje del proceso %i de tipo %i para salida de SC\n", idNodo, p.pid, p.tipo);
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
		}
		else if(p.type == 1){
			printf("Nodo %i: Recibido mensaje del proceso %i de tipo %i para entrada de SC\n", idNodo, p.pid, p.tipo);
			addPetition(p);
		}else{
			msgsnd(msqidNodo, &p, sizeof(p), 0);
		}
		sem_post(&semaforoExclusionMutua);		
	}
}

void addPetition(proceso p){
	printf("Nodo %i: Recibe peticion\n", idNodo);
	printf("\tProceso: %i\n", p.pid);
	printf("\tTipo: %i\n", p.tipo);
	
	Lista *aux = (Lista *)malloc(sizeof(Lista));
	aux->pid = p.pid;
	aux->tipo = p.tipo;
	aux->siguiente = NULL;
	numProcesos[p.tipo]++;
	printf("Nodo %i: Procesos tipo %i en el nodo: %i\n", idNodo, p.tipo, numProcesos[p.tipo]);
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
}

void *receptorInterNodo(){
	testigo t;
	int status;
	while(1){
		if(token==1)
			printf("Nodo %i: Testigo=1\n", idNodo);
		while(token==1);

		//printf("Nodo %i: Esperando por el Testigo\n", idNodo);
		status = msgrcv(msqidInterNodo, &t, sizeof(testigo), idNodo, 0);
		if(status == -1){
			printf("Nodo %i: Error al intentar recibir Testigo\n", idNodo);
			exit(0);
		}
		printf("Nodo %i: Testigo recibido\n", idNodo);

		sem_wait(&semaforoExclusionMutua);
		token = 1;
		memcpy(atendidas, t.vectorAtendidas, sizeof(atendidas));
		atendidas[idNodo] = myNum;
		if(leyendo[idNodo]>0){
			//Si hay procesos leyendo en el nodo se atienden
			//Hay que cambiar
			int i;
			for(i = 0; i<lectoresSC; i++){
				if(leyendo[i]==0){
					continue;
				}
				proceso p;
				p.pid = leyendo[i];
				p.type = leyendo[i];
				status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
			}
		}
		int numLectores = 0, nodoConLectores = 0;
		for(status = 0; status< numMaxNodos; status++){
			numLectores += t.vectorLeyendo[status];
			if(t.vectorLeyendo[status]>0){
				nodoConLectores = status;
			}
		}
		if(numLectores>0){
			//Si hay procesos leyendo en otros nodos se enviará el testigo a otro nodo para atenderlos
			sendToken(nodoConLectores);
		}
		else{
			//Si no hay procesos leyendo podemos atendemos al procesos escritor más prioritario
			if(numProcesos[tipoPago]>0){
				//Hay procesos de pagos en el nodo y se atienden el primero
				proceso p;
				p.idNodo = idNodo;
				p.pid = lista[tipoPago]->pid;
				p.type = lista[tipoPago]->pid;
				while(dentro==1);
				sem_wait(&semaforoExclusionMutuaDentro);
				dentro = 1;
				sem_post(&semaforoExclusionMutuaDentro);
				status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
				if(status == -1){
					printf("Nodo %i: Error al enviar mensaje a proceso %i\n", idNodo, lista[tipoPago]->pid);
					exit(0);
				}
				lista[tipoPago] = lista[tipoPago]->siguiente;
				numProcesos[tipoPago]--;
			}
			else if(numProcesos[tipoAnulacion]>0){
				proceso p;
				p.idNodo = idNodo;
				p.pid = lista[tipoAnulacion]->pid;
				p.type = lista[tipoAnulacion]->pid;
				while(dentro==1);
				sem_wait(&semaforoExclusionMutuaDentro);
				dentro = 1;
				sem_post(&semaforoExclusionMutuaDentro);
				status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
				if(status == -1){
					printf("Nodo %i: Error al enviar mensaje a proceso %i\n", idNodo, lista[tipoAnulacion]->pid);
					exit(0);
				}
				lista[tipoAnulacion] = lista[tipoAnulacion]->siguiente;
				numProcesos[tipoAnulacion]--;
			}
			else if(numProcesos[tipoReserva]>0){
				proceso p;
				p.idNodo = idNodo;
				p.pid = lista[tipoReserva]->pid;
				p.type = lista[tipoReserva]->pid;
				while(dentro==1);
				sem_wait(&semaforoExclusionMutuaDentro);
				dentro = 1;
				sem_post(&semaforoExclusionMutuaDentro);
				status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
				if(status == -1){
					printf("Nodo %i: Error al enviar mensaje a proceso %i\n", idNodo, lista[tipoAnulacion]->pid);
					exit(0);
				}
				lista[tipoReserva] = lista[tipoReserva]->siguiente;
				numProcesos[tipoReserva]--;
			}
			else if(numProcesos[tipoGradaEvento]>0){
				proceso p;
				p.idNodo = idNodo;
				p.pid = lista[tipoGradaEvento]->pid;
				p.type = lista[tipoGradaEvento]->pid;
				while(dentro==1);
				sem_wait(&semaforoExclusionMutuaDentro);
				dentro = 1;
				sem_post(&semaforoExclusionMutuaDentro);
				status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
				if(status == -1){
					printf("Nodo %i: Error al enviar mensaje a proceso %i\n", idNodo, lista[tipoGradaEvento]->pid);
					exit(0);
				}
				lista[tipoGradaEvento] = lista[tipoGradaEvento]->siguiente;
				numProcesos[tipoGradaEvento]--;
				leyendo[idNodo]++;
			}
		}
		sem_post(&semaforoExclusionMutua);
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