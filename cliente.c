#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>

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
void pedirTestigo(void);
void devolverTestigo(void);

int main(int argc, char* argv[]){
	int prueba = atoi(argv[1]);
	idNodo = atoi(argv[2]);
	
	char* path="/bin/ls";
	key_t key = ftok(path, idNodo);
	msqid = msgget(key, IPC_CREAT|0666);

	switch(prueba){
		case 1:
			printf("\n");
			break;
		case 2:
			printf("\n");
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
	pedirTestigo();
	printf("PAGO %i: Dentro de la SC\n", getpid());
	devolverTestigo();
	printf("PAGO %i: Fuera de la SC\n", getpid());
}

void anulacion(){
	printf("ANULACION %i: Creado proceso\n", getpid());
	printf("ANULACION %i: Intentando entrar en la SC\n", getpid());
	pedirTestigo();
	printf("ANULACION %i: Dentro de la SC\n", getpid());
	devolverTestigo();
	printf("ANULACION %i: Fuera de la SC\n", getpid());
}

void reserva(){
	printf("RESERVA %i: Creado proceso\n", getpid());
	printf("RESERVA %i: Intentando entrar en la SC\n", getpid());
	pedirTestigo();
	printf("RESERVA %i: Dentro de la SC\n", getpid());
	devolverTestigo();
	printf("RESERVA %i: Fuera de la SC\n", getpid());

}

void evento(){
	printf("EVENTO %i: Creado proceso\n", getpid());
	printf("EVENTO %i: Intentando entrar en la SC\n", getpid());
	pedirTestigo();
	printf("EVENTO %i: Dentro de la SC\n", getpid());
	devolverTestigo();
	printf("EVENTO %i; Fuera de la SC\n", getpid());
}

void grada(){
	printf("GRADA %i: Creado procesoo\n", getpid());
	printf("GRADA %i: Intentando entrar en la SC\n", getpid());
	pedirTestigo();
	printf("GRADA %i: Dentro de la SC\n", getpid());
	devolverTestigo();
	printf("GRADA %i: Fuera de la SC\n", getpid());
}

void pedirTestigo(){
	proceso p;
	p.type = 0;
	p.idNodo = idNodo;
	p.tipo = tipoPago;
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

void devolverTestigo(){
	proceso p;
	p.type = 0;
	p.idNodo = idNodo;
	p.tipo = tipoPago;
	p.pid = getpid();

	int status = msgsnd(msqid, &p, sizeof(proceso), 0);
	if(status==-1){
		printf("Error al enviar mensaje salida proceso %i\n", getpid());
		exit(0);
	}
}