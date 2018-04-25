#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main(int argc, char* argv[]) {
    int tipoLectura=0;
    int numeroNodo=0,nodo1=0,nodo2=0,nodo3=0,nodo4=0;
    int pagos=0;
    int reservas=0;
    int anulaciones=0;
    int lectores=0;
    char buffer[30];
    char string[30];
    char string2[30]="Tipo: ";
    FILE* fichero;
    FILE* fichero2;
    fichero = fopen("escritura.txt", "w+");
    int numproc= atoi(argv[1]);
    for (int i=0; i<numproc; i++){
    char cadena [30];
    int pid= i+1;
    int tipo= (rand() % 5)+1;
    int nodo= (rand() % 4)+1;
    snprintf(cadena,sizeof(cadena),"Nodo %i: \nProceso: %i\nTipo: %i\n",nodo,pid,tipo);
    fputs(cadena, fichero);
    }
    fclose(fichero);

    //leemos el fichero y analizamos:

    fichero= fopen("escritura.txt","r");
    fichero2= fopen("resumen.txt","w+");
    int j=-1;
    char *finarchivo;
    do
       {
        j++;
        finarchivo = fgets(string,30,fichero);
        int ret;
        if(string[0] == 'T'){
            tipoLectura= (int)string[6]-48;
            switch(tipoLectura){
                case 1: pagos++;
                        break;
                case 2: anulaciones++;
                        break;
                case 3: reservas++;
                        break;
                case 4: lectores++;
                        break;
                case 5: lectores++;
                        break;
                default: break;
            }

        }
        if(string[0] == 'N'){
            numeroNodo= (int)string[5]-48;
            switch(tipoLectura){
                case 1: nodo1++;
                        break;
                case 2: nodo2++;
                        break;
                case 3: nodo3++;
                        break;
                case 4: nodo4++;
                        break;
                default: break;
            }

        }
      
          //  fputs(string,fichero2);
      /*  if(ret==0){
            fputs("Tipo de proceso", fichero2);
            fputs(&string[6], fichero2);
        }*/
       // fputs(string, fichero2);
       // fputs("\n",fichero2);
       } while (finarchivo!=NULL);
      fputs("\nNumero de pagos: ",fichero2);
        sprintf(buffer,"%i",pagos);
        fputs(buffer,fichero2);
        fputs("\nNumero de anulaciones: ",fichero2);
        sprintf(buffer,"%i",anulaciones);
        fputs(buffer,fichero2);
        fputs("\nNumero de reservas: ",fichero2);
        sprintf(buffer,"%i",reservas);
        fputs(buffer,fichero2);
        fputs("\nNumero de lectores: ",fichero2);
        sprintf(buffer,"%i",lectores);
        fputs(buffer,fichero2);


        //nodos

        fputs("\nNumero de procesos en el nodo 1: ",fichero2);
        sprintf(buffer,"%i",nodo1);
        fputs(buffer,fichero2);
        fputs("\nNumero de procesos en el nodo 2: ",fichero2);
        sprintf(buffer,"%i",nodo2);
        fputs(buffer,fichero2);
        fputs("\nNumero de procesos en el nodo 3: ",fichero2);
        sprintf(buffer,"%i",nodo3);
        fputs(buffer,fichero2);
        fputs("\nNumero de procesos en el nodo 4: ",fichero2);
        sprintf(buffer,"%i",nodo4);
        fputs(buffer,fichero2);


    printf("El archivo tiene %d valores",j);
    fclose (fichero);
    fclose (fichero2);  

    printf("Proceso completado\n");
    return 0;
}