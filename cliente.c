#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/wait.h>

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
			printf("Prueba 1: 100 procesos de eventos y 200 de gradas en un nodo\n");
			for(i=0; i<100; i++){
				pid = fork();
				if(pid == -1){
					printf("Error al crear proceso hijo");
					exit(0);
				}
				else if(pid == 0){
					evento();
				}
			}
			for(i=0; i<200; i++){
				pid = fork();
				if(pid == -1){
					printf("Error al crear proceso hijo");
					exit(0);
				}
				else if(pid == 0){
					grada();
				}
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
				}
			}
			for(i=0; i<200; i++){
				wait(&pid);
			}
			break;
		case 3:
			printf("\n");
			break;
		case 4:
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
	printf("PAGO %i: Intentando entrar en la SC\n", getpid());
	entradaSC(tipoPago);
	printf("PAGO %i: Dentro de la SC\n", getpid());
	salidaSC(tipoPago);
	printf("PAGO %i: Fuera de la SC\n", getpid());
}

void anulacion(){
	printf("ANULACION %i: Creado proceso\n", getpid());
	printf("ANULACION %i: Intentando entrar en la SC\n", getpid());
	entradaSC(tipoAnulacion);
	printf("ANULACION %i: Dentro de la SC\n", getpid());
	salidaSC(tipoAnulacion);
	printf("ANULACION %i: Fuera de la SC\n", getpid());
}

void reserva(){
	printf("RESERVA %i: Creado proceso\n", getpid());
	printf("RESERVA %i: Intentando entrar en la SC\n", getpid());
	entradaSC(tipoReserva);
	printf("RESERVA %i: Dentro de la SC\n", getpid());
	salidaSC(tipoReserva);
	printf("RESERVA %i: Fuera de la SC\n", getpid());

}

void evento(){
	printf("EVENTO %i: Creado proceso\n", getpid());
	printf("EVENTO %i: Intentando entrar en la SC\n", getpid());
	entradaSC(tipoGradaEvento);
	printf("EVENTO %i: Dentro de la SC\n", getpid());
	salidaSC(tipoGradaEvento);
	printf("EVENTO %i; Fuera de la SC\n", getpid());
}

void grada(){
	printf("GRADA %i: Creado procesoo\n", getpid());
	printf("GRADA %i: Intentando entrar en la SC\n", getpid());
	entradaSC(tipoGradaEvento);
	printf("GRADA %i: Dentro de la SC\n", getpid());
	salidaSC(tipoGradaEvento);
	printf("GRADA %i: Fuera de la SC\n", getpid());
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