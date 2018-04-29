#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/time.h>

typedef struct{
	long type;
	int idNodo;
	int tipo;
	int pid;
}proceso;

//Tipos de procesos según prioridades
#define tipoPago 4
#define tipoAnulacion 3
#define tipoReserva 2
#define tipoGradaEvento 1

//Definición ids de los nodos
#define idNodo1 1
#define idNodo2 2
#define idNodo3 3
#define idNodo4 4

//Definición variables
int msqid1 = 0;
int msqid2 = 0;
int msqid3 = 0;
int msqid4 = 0;


//Definición de cabeceras
void pago(int, int);
void anulacion(int, int);
void reserva(int, int);
void evento(int, int);
void grada(int, int);
void entradaSC(int, int, int);
void salidaSC(int, int, int);
long long int getTimestamp(void);

int main(int argc, char* argv[]){
	char* path="/bin/ls";
	key_t key1 = ftok(path, idNodo1);
	msqid1 = msgget(key1, IPC_CREAT|0666);
	if(msqid1==-1){
		printf("Error al crear cola de mensajes del nodo 1\n");
		exit(0);
	}
	key_t key2 = ftok(path, idNodo2);
	msqid2 = msgget(key2, IPC_CREAT|0666);
	if(msqid2 == -1){
		printf("Error al crear cola de mensajes del nodo 2\n");
		exit(0);
	}
	key_t key3 = ftok(path, idNodo3);
	msqid3 = msgget(key3, IPC_CREAT|0666);
	if(msqid3 == -1){
		printf("Error al crear cola de mensajes del nodo 3\n");
		exit(0);
	}
	key_t key4 = ftok(path, idNodo4);
	msqid4 = msgget(key4, IPC_CREAT|0666);
	if(msqid4 == -1){
		printf("Error al crear cola de mensajes del nodo 4\n");
		exit(0);
	}

	struct sched_param param;
	param.sched_priority = 90;
	sched_setscheduler(getpid(), SCHED_FIFO, &param);

	int pid, i;
	for(i=0; i<20; i++){
		pid = fork();
		if(pid == -1){
			printf("Error al crear proceso hijo\n");
			exit(0);
		}
		else if(pid == 0){
			param.sched_priority = 70;
			sched_setscheduler(getpid(), SCHED_FIFO, &param);
			reserva(idNodo1, msqid1);
			return 0;
		}
	}
	for(i=0; i<10; i++){
		pid = fork();
		if(pid == -1){
			printf("Error al crear proceso hijo\n");
			exit(0);
		}
		else if(pid == 0){
			param.sched_priority = 70;
			sched_setscheduler(getpid(), SCHED_FIFO, &param);
			pago(idNodo3, msqid3);
			return 0;
		}
	}

	for(i=0; i<27; i++)
		wait(&pid);
	
	for(i=0; i<15; i++){
		pid = fork();
		if(pid == -1){
			printf("Error al crear proceso hijo\n");
			exit(0);
		}
		else if(pid == 0){
			param.sched_priority = 70;
			sched_setscheduler(getpid(), SCHED_FIFO, &param);
			anulacion(idNodo2, msqid2);
			return 0;
		}
	}

	for(i=0; i<16; i++)
		wait(&pid);

	for(i=0; i<6; i++){
		pid = fork();
		if(pid == -1){
			printf("Error al crear proceso hijo\n");
			exit(0);
		}
		else if(pid == 0){
			sched_setscheduler(getpid(), SCHED_FIFO, &param);
			grada(idNodo4, msqid4);
			return 0;
		}
	}

	for(i=0; i<6; i++){
		pid = fork();
		if(pid == -1){
			printf("Error al crear proceso hijo\n");
			exit(0);
		}
		else if(pid == 0){
			sched_setscheduler(getpid(), SCHED_FIFO, &param);
			evento(idNodo1, msqid1);
			return 0;
		}
	}

	for(i=0; i<12; i++)
		wait(&pid);

	for(i=0; i<10; i++){
		pid = fork();
		if(pid == -1){
			printf("Error al crear proceso hijo\n");
			exit(0);
		}
		else if(pid == 0){
			sched_setscheduler(getpid(), SCHED_FIFO, &param);
			anulacion(idNodo1, msqid1);
			return 0;
		}
	}

	for(i=0; i<12; i++)
		wait(&pid);
	
	/*

	switch(prueba){
		case 1:
			printf("Prueba 1: 10 procesos de eventos y 20 de gradas en un nodo\n");
			for(i=0; i<5; i++){
				pid = fork();
				if(pid == -1){
					printf("Error al crear proceso hijo");
					exit(0);
				}
				else if(pid == 0){
					usleep(100000);
					evento();
					return 0;
				}
			}
			for(i=0; i<4; i++){
				pid = fork();
				if(pid == -1){
					printf("Error al crear proceso hijo");
					exit(0);
				}
				else if(pid == 0){
					usleep(100000);
					grada();
					return 0;
				}
			}
			for(i=0; i<7; i++){
				wait(&pid);
			}
			break;
		case 2:
			printf("Prueba 2: 100 procesos de pagos\n");
			for(i=0; i<100; i++){
				pid = fork();
				if(pid == -1){
					printf("Error al crear proceso hijo");
					exit(0);
				}
				else if(pid == 0){
					pago();
					return 0;
				}
			}
			for(i=0; i<100; i++){
				wait(&pid);
			}
			break;
		case 3:
			printf("Prueba 3: 100 procesos de reservas\n");
			for(i=0; i<100; i++){
				pid = fork();
				if(pid==0){
					usleep(1000);
					reserva();
					return 0;
				}
			}
			// pid = fork();
			// if(pid ==0){
			// 	usleep(1000);
			// 	anulacion();
			// 	return 0;
			// }
			// pid = fork();
			// if(pid==0){
			// 	usleep(990);
			// 	pago();
			// 	return 0;
			// }
			for (i=0;i<100;i++){
				wait(&pid);
			}

			printf("\n");
			break;
		case 4:
		printf("Prueba 4: 100 procesos de anulaciones\n");
			for(i=0; i<100; i++){
				pid = fork();
				if(pid==0){
					usleep(1000);
					anulacion();
					return 0;
				}
			}
			
			for (i=0;i<100;i++){
				wait(&pid);
			}

			printf("\n");
			break;
		case 5:
		printf("Prueba 5: 100 procesos de pagos, 100 de anulaciones y 100 de reservas\n");
			for(i=0; i<100; i++){
				pid = fork();
				if(pid==0){
					usleep(1000);
					pago();
					return 0;
				}
			}
			for(i=0; i<100; i++){
				pid = fork();
				if(pid==0){
					usleep(1000);
					anulacion();
					return 0;
				}
			}
			for(i=0; i<100; i++){
				pid = fork();
				if(pid==0){
					usleep(1000);
					reserva();
					return 0;
				}
			}
			for (i=0;i<300;i++){
				wait(&pid);
			}



			printf("\n");
			break;
		case 6:
			printf("\n");
			break;
		case 7:
			printf("\n");
			break;
		case 8:
			printf("\n");
			break;
		case 9:
			printf("\n");
			break;
		case 10:
			printf("\n");
			break;
		default:
			printf("Prueba no definida\n");
			
	}*/
	return 0;
}

void pago(int idNodo, int msqidNodo){
	printf("PAGO %i\t[%lld][Nodo %i]:\tCreado proceso\n", getpid(), getTimestamp(), idNodo);
	printf("PAGO %i\t[%lld][Nodo %i]:\tIntento acceder a SC\n", getpid(), getTimestamp(), idNodo);
	entradaSC(tipoPago, idNodo, msqidNodo);
	printf("PAGO %i\t[%lld][Nodo %i]:\tDentro de la SC\n", getpid(), getTimestamp(), idNodo);
	sleep(1);
	salidaSC(tipoPago, idNodo, msqidNodo);
	printf("PAGO %i\t[%lld][Nodo %i]:\tFuera de la SC\n", getpid(), getTimestamp(), idNodo);
}

void anulacion(int idNodo, int msqidNodo){
	printf("ANULACION %i\t[%lld][Nodo %i]:\tCreado proceso\n", getpid(), getTimestamp(), idNodo);
	printf("ANULACION %i\t[%lld][Nodo %i]:\tIntento acceder a SC\n", getpid(), getTimestamp(), idNodo);
	entradaSC(tipoAnulacion, idNodo, msqidNodo);
	printf("ANULACION %i\t[%lld][Nodo %i]:\tDentro de la SC\n", getpid(), getTimestamp(), idNodo);
	sleep(1);
	salidaSC(tipoAnulacion, idNodo, msqidNodo);
	printf("ANULACION %i\t[%lld][Nodo %i]:\tFuera de la SC\n", getpid(), getTimestamp(), idNodo);
}

void reserva(int idNodo, int msqidNodo){
	printf("RESERVA %i\t[%lld][Nodo %i]:\tCreado proceso\n", getpid(), getTimestamp(), idNodo);
	printf("RESERVA %i\t[%lld][Nodo %i]:\tIntento acceder a SC\n", getpid(), getTimestamp(), idNodo);
	entradaSC(tipoReserva, idNodo, msqidNodo);
	printf("RESERVA %i\t[%lld][Nodo %i]:\tDentro de la SC\n", getpid(), getTimestamp(), idNodo);
	sleep(1);
	salidaSC(tipoReserva, idNodo, msqidNodo);
	printf("RESERVA %i\t[%lld][Nodo %i]:\tFuera de la SC\n", getpid(), getTimestamp(), idNodo);

}

void evento(int idNodo, int msqidNodo){
	printf("EVENTO %i\t[%lld][Nodo %i]:\tCreado proceso\n", getpid(), getTimestamp(), idNodo);
	printf("EVENTO %i\t[%lld][Nodo %i]:\tIntento acceder a SC\n", getpid(), getTimestamp(), idNodo);
	entradaSC(tipoGradaEvento, idNodo, msqidNodo);
	printf("EVENTO %i\t[%lld][Nodo %i]:\tDentro de la SC\n", getpid(), getTimestamp(), idNodo);
	sleep(5);
	salidaSC(tipoGradaEvento, idNodo, msqidNodo);
	printf("EVENTO %i\t[%lld][Nodo %i]:\tFuera de la SC\n", getpid(), getTimestamp(), idNodo);
}

void grada(int idNodo, int msqidNodo){
	printf("GRADA %i\t[%lld][Nodo %i]:\tCreado procesoo\n", getpid(), getTimestamp(), idNodo);
	printf("GRADA %i\t[%lld][Nodo %i]:\tIntento acceder a SC\n", getpid(), getTimestamp(), idNodo);
	entradaSC(tipoGradaEvento, idNodo, msqidNodo);
	printf("GRADA %i\t[%lld][Nodo %i]:\tDentro de la SC\n", getpid(), getTimestamp(), idNodo);
	sleep(2);
	salidaSC(tipoGradaEvento, idNodo, msqidNodo);
	printf("GRADA %i\t[%lld][Nodo %i]:\tFuera de la SC\n", getpid(), getTimestamp(), idNodo);
}

//Si se solicita la entrada a la SC se enviará un mensaje de tipo 1
void entradaSC(int tipoProceso, int idNodo, int msqidNodo){
	proceso p;
	p.type = 1;
	p.idNodo = idNodo;
	p.tipo = tipoProceso;
	p.pid = getpid();

	int status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
	if(status==-1){
		printf("Error al enviar mensaje entrada proceso %i\n", getpid());
		exit(0);
	}
	status = msgrcv(msqidNodo, &p, sizeof(proceso), getpid(), 0);
	if(status==-1){
		printf("Error al recibir mensaje proceso %i\n", getpid());
		exit(0);
	}
}

//Si se notifica la salida a la SC se envia un mensaje de tipo 2
void salidaSC(int tipoProceso,int idNodo, int msqidNodo){
	proceso p;
	p.type = 2;
	p.idNodo = idNodo;
	p.tipo = tipoProceso;
	p.pid = getpid();

	int status = msgsnd(msqidNodo, &p, sizeof(proceso), 0);
	if(status==-1){
		printf("Error al enviar mensaje salida proceso %i\n", getpid());
		exit(0);
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