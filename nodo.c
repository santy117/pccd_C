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
sem_t semaforoExclusionMutuaDentro;

//Variables intranodo
int idNodo;
int msqidNodo;
int msqidInterNodo;
int idOtrosNodos[numMaxNodos] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int peticiones[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int atendidas[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int leyendo[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int prioridades[numMaxNodos+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int token;
int dentro;
int myNum;
int nodos;

int numProcesos[5] = {0, 0, 0, 0, 0};
Lista *lista[5];
int procLeyendo[lectoresSC];


//Cabeceras funciones
void *receptorInterNodo(void);
void *receptorNodo(void);
void requestToken(void);
void sendToken(int);
void asignToken(void);
void addPetition(proceso);


int main(int argc, char* argv[]){
	int i;
	for(i=0; i<lectoresSC+1; i++){
		procLeyendo[i] = 0;
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
	if(numProcesos[tipoPago]>0){
		request.prioridad = tipoPago;
		request.num = numProcesos[tipoPago];
	}
	else if(numProcesos[tipoAnulacion]>0){
		request.prioridad = tipoAnulacion;
		request.num = numProcesos[tipoAnulacion];
	}
	else if(numProcesos[tipoReserva]>0){
		request.prioridad = tipoReserva;
		request.num = numProcesos[tipoReserva];
	}
	else if(numProcesos[tipoGradaEvento]>0){
		request.prioridad = tipoGradaEvento;
		request.num = numProcesos[tipoGradaEvento];
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
	t.vectorAtendidas = atendidas;
	t.vectorLeyendo = leyendo;
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
		status = msgrcv(msqidNodo, &p, sizeof(proceso), 0, 0);
		if(status == -1){
			printf("Nodo %i: Error al recibir mensaje cola nodo\n", idNodo);
			exit(0);
		}
		sem_wait(&semaforoExclusionMutua);
		if(p.type == 2){
			printf("Nodo %i: Recibido mensaje del proceso %i de tipo %i para salida de SC", idNodo, p.pid, p.tipo);
			sem_wait(&semaforoExclusionMutuaDentro);
			dentro = 0;
			sem_post(&semaforoExclusionMutuaDentro);
		}
		else if(p.type == 1){
			printf("Nodo %i: Recibido mensaje del proceso %i de tipo %i para entrada de SC", idNodo, p.pid, p.tipo);
			addPetition(p);
		}else{
			msgsnd(msqidNodo, &p, sizeof(p), 0);
		}
		sem_post(&semaforoExclusionMutua);		
	}
}

void addPetition(proceso p){
	printf("Nodo %i: añadida petición\n", idNodo);
	proceso pRespuesta;
	pRespuesta.type = p.pid;
	pRespuesta.pid = p.pid;
	int status = msgsnd(msqidNodo, &pRespuesta, sizeof(proceso), 0);

	if(status == -1){
		printf("Error\n");
		exit(0);
	}
}

void *receptorInterNodo(){
	testigo t;
	int status;
	while(1){
		if(token==1)
			printf("Nodo %i: Testigo\n", idNodo);
		while(token==1);

		printf("Nodo %i: Esperando por el Testigo\n", idNodo);
		status = msgrcv(msqidInterNodo, &t, sizeof(testigo), 0, 0);
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
			int i;
			for(i = 0; i<lectoresSC; i++){
				if(procLeyendo[i]==0){
					continue;
				}
				proceso p;
				p.pid = procLeyendo[i];
				p.type = procLeyendo[i];
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
			//Si no hay procesos leyendo podemos atender a los procesos escritores
			//Variables en las que se comprar si otros nodos tienen procesos prioritarios
			int pPagos= 0, pAnulaciones = 0, pReservas=0, pGradasEventos=0,i;
			for(i=0; i<numMaxNodos+1; i++){
				if(prioridades[i]==tipoPago){
					pPagos = i;
				}
				else if(prioridades[i]==tipoAnulacion){
					pAnulaciones = i;
				}
				else if(prioridades[i]==tipoReserva){
					pReservas = i;
				}
				else if(prioridades[i]==tipoGradaEvento){
					pGradasEventos = i;
				}
			}
			if(numProcesos[tipoPago]>0){
				//Hay procesos de pagos en el nodo y se atienden
				proceso p;
				p.idNodo = idNodo;
				while(lista[tipoPago]!= NULL){
					p.pid = lista[tipoPago]->pid;
					p.type = lista[tipoPago]->pid;
					while(dentro==1);
					sem_wait(&semaforoExclusionMutuaDentro);
					dentro = 1;
					sem_post(&semaforoExclusionMutuaDentro);
					status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
					if(status == -1){
						printf("Nodo %i: Error al enviar mensaje para permitir entrar en la SC al proceso %i\n", idNodo, lista[tipoPago]->pid);
						exit(0);
					}
					lista[tipoPago] = lista[tipoPago]->siguiente;
					numProcesos[tipoPago]--; //Decrementamos el contador de procesos de pago
				}
			}
			if(pPagos!=0){
				//Hay otro nodo con procesos de tipo pago sin atender
				sendToken(idOtrosNodos[pPagos]);
				continue;

			}
			if(numProcesos[tipoAnulacion]>0){
				//Hay procesos de anulaciones en el y nodo se atienden
				proceso p;
				p.idNodo = idNodo;
				while(lista[tipoAnulacion]!=NULL){
					p.pid = lista[tipoAnulacion]->pid;
					p.type = lista[tipoAnulacion]->pid;
					while(dentro==1);
					sem_wait(&semaforoExclusionMutuaDentro);
					dentro = 1;
					sem_post(&semaforoExclusionMutuaDentro);
					status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
					if(status == -1){
						printf("Nodo %i: Error l enviar mensaje para permitir entrar en la SC al proceso %i\n", idNodo, lista[tipoAnulacion]->pid);
						exit(0);
					}		
					lista[tipoAnulacion] = lista[tipoAnulacion]->siguiente;
					numProcesos[tipoAnulacion]--;
				}
			}
			if(pAnulaciones!=0){
				//Hay procesos de anulaciones en otros nodos
				sendToken(idOtrosNodos[pReservas]);
				continue;
			}
			if(numProcesos[tipoReserva]>0){
				//Hay procesos de reservas en el nodo y se atienten
				proceso p;
				p.idNodo = idNodo;
				while(lista[tipoReserva]!=NULL){
					p.pid = lista[tipoReserva]->pid;
					p.type = lista[tipoReserva]->pid;
					while(dentro==1);
					sem_wait(&semaforoExclusionMutuaDentro);
					dentro = 1;
					sem_wait(&semaforoExclusionMutuaDentro);
					status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
					if(status == -1){
						printf("Nodo %i: Error l enviar mensaje para permitir entrar en la SC al proceso %i\n", idNodo, lista[tipoReserva]->pid);
						exit(0);
					}
				}
			}
			if(pReservas!=0){
				//Hay procesos de reservas en otros nodos
				sendToken(idOtrosNodos[pReservas]);
				continue;
			}
			if(pGradasEventos!=0){
				sendToken(idOtrosNodos[pGradasEventos]);
				continue;
			}
		}
		sem_post(&semaforoExclusionMutua);
	}
}