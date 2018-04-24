#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
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

//Definición variables

int idNodo = 0;
int msqid = 0;

//Definición de cabeceras
void pago(void);
void anulacion(void);
void reserva(void);
void evento(void);
void grada(void);
void entradaSC(int);
void salidaSC(int);
long long int getTimestamp(void);

int main(int argc, char* argv[]){
	int prueba = atoi(argv[1]);
	idNodo = atoi(argv[2]);
	
	char* path="/bin/ls";
	key_t key = ftok(path, idNodo);
	msqid = msgget(key, IPC_CREAT|0666);
	if(msqid==-1){
		printf("Error al crear cola de mensajes\n");
	}
	int pid, i;
	switch(prueba){
		case 1:
			printf("Prueba 1: 10 procesos de eventos y 20 de gradas en un nodo\n");
			for(i=0; i<3; i++){
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
			printf("Prueba 2: 200 prrocesos de pagos\n");
			for(i=0; i<200; i++){
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
			for(i=0; i<200; i++){
				wait(&pid);
			}
			break;
		case 3:
			pid = fork();
			if(pid==0){
				usleep(1000);
				reserva();
				return 0;
			}
			pid = fork();
			if(pid ==0){
				usleep(1000);
				anulacion();
				return 0;
			}
			pid = fork();
			if(pid==0){
				usleep(990);
				pago();
				return 0;
			}
			wait(&pid);
			wait(&pid);
			wait(&pid);

			printf("\n");
			break;
		case 4:
			pid = fork();
			if(pid==0){
				usleep(1000);
				grada();
				return 0;
			}
			pid = fork();
			if(pid ==0){
				usleep(1000);
				evento();
				return 0;
			}
			pid = fork();
			if(pid==0){
				usleep(50000);
				pago();
				return 0;
			}
			pid = fork();
			if(pid==0){
				usleep(50000);
				grada();
				return 0;
			}
			wait(&pid);
			wait(&pid);
			wait(&pid);
			wait(&pid);

			printf("\n");
			break;
		case 5:
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
			
	}
	return 0;
}


void pago(){
	printf("PAGO %i: Creado proceso\n", getpid());
	printf("PAGO %i: Permiso para SC\t%lld\n", getpid(), getTimestamp());
	entradaSC(tipoPago);
	printf("PAGO %i: Dentro de la SC\t%lld\n", getpid(), getTimestamp());
	usleep(1);
	salidaSC(tipoPago);
	printf("PAGO %i: Fuera de la SC\t%lld\n", getpid(), getTimestamp());
}

void anulacion(){
	printf("ANULACION %i: Creado proceso\n", getpid());
	printf("ANULACION %i: Permiso para SC\t%lld\n", getpid(), getTimestamp());
	entradaSC(tipoAnulacion);
	printf("ANULACION %i: Dentro de la SC\t%lld\n", getpid(), getTimestamp());
	usleep(1);
	salidaSC(tipoAnulacion);
	printf("ANULACION %i: Fuera de la SC\t%lld\n", getpid(), getTimestamp());
}

void reserva(){
	printf("RESERVA %i: Creado proceso\n", getpid());
	printf("RESERVA %i: Permiso para SC\t%lld\n", getpid(), getTimestamp());
	entradaSC(tipoReserva);
	printf("RESERVA %i: Dentro de la SC\t%lld\n", getpid(), getTimestamp());
	usleep(1);
	salidaSC(tipoReserva);
	printf("RESERVA %i: Fuera de la SC\t%lld\n", getpid(), getTimestamp());

}

void evento(){
	printf("EVENTO %i: Creado proceso\n", getpid());
	printf("EVENTO %i: Permiso para SC\t%lld\n", getpid(), getTimestamp());
	entradaSC(tipoGradaEvento);
	printf("EVENTO %i: Dentro de la SC\t%lld\n", getpid(), getTimestamp());
	sleep(1);
	salidaSC(tipoGradaEvento);
	printf("EVENTO %i; Fuera de la SC\t%lld\n", getpid(), getTimestamp());
}

void grada(){
	printf("GRADA %i: Creado procesoo\n", getpid());
	printf("GRADA %i: Permiso para SC\t%lld\n", getpid(), getTimestamp());
	entradaSC(tipoGradaEvento);
	printf("GRADA %i: Dentro de la SC\t%lld\n", getpid(), getTimestamp());
	usleep(5000);
	salidaSC(tipoGradaEvento);
	printf("GRADA %i: Fuera de la SC\t%lld\n", getpid(), getTimestamp());
}

//Si se solicita la entrada a la SC se enviará un mensaje de tipo 1
void entradaSC(int tipoProceso){
	proceso p;
	p.type = 1;
	p.idNodo = idNodo;
	p.tipo = tipoProceso;
	p.pid = getpid();

	int status = msgsnd(msqid, &p, sizeof(proceso), 0);
	if(status==-1){
		printf("Error al enviar mensaje entrada proceso %i\n", getpid());
		exit(0);
	}
	status = msgrcv(msqid, &p, sizeof(proceso), getpid(), 0);
	if(status==-1){
		printf("Error al recibir mensaje proceso %i\n", getpid());
		exit(0);
	}
}

//Si se notifica la salida a la SC se envia un mensaje de tipo 2
void salidaSC(int tipoProceso){
	proceso p;
	p.type = 2;
	p.idNodo = idNodo;
	p.tipo = tipoProceso;
	p.pid = getpid();

	int status = msgsnd(msqid, &p, sizeof(proceso), 0);
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