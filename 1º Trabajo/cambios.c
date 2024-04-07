#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include "cambios.h"

#define NUM_HIJOS 32
#define TAMANO_MEMORIA 100

// Estructura para el semáforo
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct Mensaje {
    long tipo;
    int grupoActual;

    int posGrupoDeseado;
    int flag;
}mensaje;

struct Memoria {
    char shared_memory[TAMANO_MEMORIA];
    int cont;
    int indicadorProc;
}*memoria_compartida;
int semid;
int msgid;
int shmid;
union semun valor;

void placeMemory(int hijo) {
    char letra = LETRAS[hijo];
    int posicion = -1;
    int valor = -1;
    int i;

    switch (letra) {
        case 'A': case 'B': case 'C': case 'D': case 'a': case 'b': case 'c': case 'd':
            valor = 1;
        break;
        case 'E': case 'F': case 'G': case 'H': case 'e': case 'f': case 'g': case 'h':
            valor = 2;
        break;
        case 'I': case 'J': case 'L': case 'M': case 'i': case 'j': case 'l': case 'm':
            valor = 3;
        break;
        case 'N': case 'O': case 'P': case 'R': case 'n': case 'o': case 'p': case 'r':
            valor = 4;
        break;
    }

    if (valor != -1) {
        for (i = 0; i < TAMANO_MEMORIA; i += 2) {
            if (memoria_compartida->shared_memory[i] == ' ' && memoria_compartida->shared_memory[i + 1] == valor) {
                posicion = i;
                break;
            }
        }
    }

    if (posicion != -1) {
        memoria_compartida->shared_memory[posicion] = letra;
    }

    //return posicion;
}

void deleteIPC(int flag) {
    if(flag == 1) {
        // Eliminar los hijos
        kill(0, SIGTERM);
        int i;

        for(i = 0; i < NUM_HIJOS; i++) {
            wait(NULL);
        }
    }
    semctl(semid, 0, IPC_RMID);
    shmctl(shmid, IPC_RMID, NULL);
    msgctl(msgid, IPC_RMID, NULL);

    exit(0);
}

void sigintHandler(int sig) {
    deleteIPC(1);
}

void alarmHandler(int signal) {
    kill(0, SIGTERM);
    int i;

    for(i = 0; i < NUM_HIJOS; i++) {
        wait(NULL);
    }

    finCambios(); 
    deleteIPC(0);
    exit(0);
}

int isNumber(const char* str) {
    char* endptr;
    strtol(str, &endptr, 10);

    if (*endptr == '\0') {
        return 1;
    } else {
        return 0;
    }
}

int findLetterPos(int i) {
    int j;
    for(j = 0; j < TAMANO_MEMORIA; j += 2) {
        if(memoria_compartida->shared_memory[j] == LETRAS[i]) {
            return j;
        }
    }
    return -1;
}

int main(int argc, char const *argv[]) {
    pid_t pid;
    int i;
    srand(time(NULL));

    // Comprobar parámetros
    if(argc != 2) {
        printf("Modo de uso >> '%s <retardo>'\n", argv[0]);
        return 1;
    }
    if (!isNumber(argv[1])) {
        printf("Error: '%s' no es un número válido\n", argv[1]);
        return 1;
    }
    int retardo = atoi(argv[1]);
    if (retardo < 0) {
        printf("Error: El número debe ser mayor o igual a 0\n");
        return 1;
    }

    // Registrar la alarma
    struct sigaction sa;
    sa.sa_handler = alarmHandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGALRM, &sa, NULL) == -1) {
        printf("Error al configurar la alarma");
        return 1;
    }
    int alarma;
    if(retardo == 0) {
        alarma = 47;
    } else {
        alarma = 57;
    }
    alarm(alarma);

    // Registrar el manejador de señales para SIGINT
    struct sigaction ss;

    ss.sa_handler = sigintHandler;
    sigfillset(&ss.sa_mask);
    ss.sa_flags = 0;
    if(sigaction(SIGINT, &ss, NULL) == -1) {
        printf("Error al configurar señal SIGINT");
        return 1;
    }

    // Crear memoria compartida
    shmid = shmget(IPC_PRIVATE, sizeof(struct Memoria), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }

    // Adjuntar la memoria compartida
    memoria_compartida = (struct Memoria *)shmat(shmid, NULL, 0);
    if (memoria_compartida == (void *) -1) {
        perror("Error al adjuntar memoria compartida");
        exit(1);
    }

    // Crear semáforo
    if ((semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666)) == -1) {
        perror("Error al crear semáforo");
        exit(1);
    }

    // Inicializar el semáforo
    valor.val = 1;
    if (semctl(semid, 1, SETVAL, valor) == -1) {
        perror("Error al inicializar el semáforo 1");
        exit(1);
    }
    valor.val = 0;
    if (semctl(semid, 2, SETVAL, valor) == -1) {
        perror("Error al inicializar el semáforo 2");
        exit(1);
    }
    valor.val = 1;
    if (semctl(semid, 3, SETVAL, valor) == -1) {
        perror("Error al inicializar el semáforo 3");
        exit(1);
    }

    // Crear buzon
    msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("Error al crear la cola de mensajes");
        return 1;
    }

    inicioCambios(retardo, semid, memoria_compartida->shared_memory);

    // Inicializar la memoria compartida con espacios en blanco
    for (i = 1; i < TAMANO_MEMORIA; i += 2) {
        if (i < 16) {
            memoria_compartida->shared_memory[i] = 1;
        } else if (i > 20 && i < 36) {
            memoria_compartida->shared_memory[i] = 2;
        } else if (i > 40 && i < 56) {
            memoria_compartida->shared_memory[i] = 3;
        } else if (i > 60 && i < 76) {
            memoria_compartida->shared_memory[i] = 4;
        }
    }
    int *cambioConcedido;
    cambioConcedido = (int *)&(memoria_compartida->shared_memory[84]);
    *(cambioConcedido) = 0;

    // Crear hijos y colocar su número en la memoria compartida
    char letra;
    int grupo;
    int posLetra;
    int cambio;

    memoria_compartida->cont = 0;
    memoria_compartida->indicadorProc = 0;

    struct sembuf operacion;

    for (i = 0; i < NUM_HIJOS; i++) {
        pid = fork();
        if (pid == 0) { // Proceso hijo

            // Colocar el número del hijo en la memoria compartida
            placeMemory(i);
            refrescar();

            memoria_compartida->cont++;
            //memoria_compartida->indicadorProc++;

            while(memoria_compartida->cont < 31) {
                wait(0);
            }        

            while(1) {
                operacion.sem_num = 1;
                operacion.sem_op = -1;
                operacion.sem_flg = 0;
                if(semop(semid, &operacion, 1) == -1) {
                    perror("Error al hacer wait semaforo 1");
                    return 0;
                }
                posLetra = findLetterPos(i);
                letra = memoria_compartida->shared_memory[posLetra];
                grupo = memoria_compartida->shared_memory[posLetra + 1];

                cambio = aQuEGrupo(grupo);
                memoria_compartida->shared_memory[posLetra + 1] = cambio;

                refrescar();

                mensaje.tipo = i + 1;
                mensaje.grupoActual = grupo;
                mensaje.flag = 0;

                if(msgsnd(msgid, &mensaje, sizeof(mensaje) - sizeof(long), 0) == -1) {
                    perror("Error al enviar mensaje");
                    return 1;
                }
                operacion.sem_num = 2;
                operacion.sem_op = 1;
                operacion.sem_flg = 0;
                if(semop(semid, &operacion, 1) == -1) {
                    perror("Error al hacer signal semaforo 1");
                    return 0;
                }
                if(msgrcv(msgid, &mensaje, sizeof(mensaje) - sizeof(long), mensaje.tipo, 0) == -1) {
                    perror("Error al recibir mensaje");
                    return 1;
                }


                if(mensaje.flag == 0) {
                    //Se ha concedido el cambio
                    char letraCambio = memoria_compartida->shared_memory[mensaje.posGrupoDeseado];
                    char letraAux = letra;
                    //char cadena[70];
                    //sprintf(cadena, "LetraAux %c   LetraCambio %c   Grupo %d   Cambio%d", letraAux, letraCambio, grupo, cambio);
                    //pon_error(cadena);

                    memoria_compartida->shared_memory[posLetra] = letraCambio;
                    memoria_compartida->shared_memory[posLetra + 1] = grupo;

                    memoria_compartida->shared_memory[mensaje.posGrupoDeseado] = letraAux;
                    memoria_compartida->shared_memory[mensaje.posGrupoDeseado + 1] = cambio;

                    *(cambioConcedido) += 1;
                    incrementarCuenta();
                } else {
                    //No se ha concedido el cambio
                }

                operacion.sem_num = 1;
                operacion.sem_op = 1;
                operacion.sem_flg = 0;
                if(semop(semid, &operacion, 1) == -1) {
                    perror("Error al hacer signal semaforo 3");
                    return 0;
                }
            }

            exit(0);
        } else if (pid < 0) {
            printf("Error al crear el hijo %d\n", i);
            exit(1);
        }
    }

    // PADRE
    int grupoDeseado;
    int posLetraCambio;
    while(1) {
        operacion.sem_num = 2;
        operacion.sem_op = -1;
        operacion.sem_flg = 0;
        if(semop(semid, &operacion, 1) == -1) {
            perror("Error al hacer wait semaforo 2");
            return 0;
        }
        if(msgrcv(msgid, &mensaje, sizeof(mensaje) - sizeof(long), 0, 0) == -1) {
            perror("Error al recibir mensaje");
            return 1;
        }

        posLetra = findLetterPos(mensaje.tipo - 1);
        grupoDeseado = memoria_compartida->shared_memory[posLetra + 1];

        //printf("Mensaje tipo %ld letra %c%d quiere ir a %d\n", mensaje.tipo, memoria_compartida->shared_memory[posLetra], mensaje.grupoActual, grupoDeseado);

        if(grupoDeseado == 1) {
            for(i = 1; i < 16; i+=2) {
                if(memoria_compartida->shared_memory[i] == mensaje.grupoActual){
                    posLetraCambio = i - 1;
                } else {
                    posLetraCambio = -1;
                }
            }
        } else if(grupoDeseado == 2) {
            for(i = 21; i < 36; i+=2) {
                if(memoria_compartida->shared_memory[i] == mensaje.grupoActual){
                    posLetraCambio = i - 1;
                } else {
                    posLetraCambio = -1;
                }
            }
        } else if(grupoDeseado == 3) {
            for(i = 41; i < 56; i+=2) {
                if(memoria_compartida->shared_memory[i] == mensaje.grupoActual){
                    posLetraCambio = i - 1;
                } else {
                    posLetraCambio = -1;
                }
            }
        } else if(grupoDeseado == 4) {
            for(i = 61; i < 76; i+=2) {
                if(memoria_compartida->shared_memory[i] == mensaje.grupoActual){
                    posLetraCambio = i - 1;
                } else {
                    posLetraCambio = -1;
                }
            }
        }

        if(posLetraCambio != -1) {
            //printf("%c%d se puede cambiar a %c%d\n", memoria_compartida->shared_memory[posLetra], memoria_compartida->shared_memory[posLetra+1],memoria_compartida->shared_memory[posLetraCambio], memoria_compartida->shared_memory[posLetraCambio+1]);
            mensaje.posGrupoDeseado = posLetraCambio;
            mensaje.flag = 0;
        } else {
            mensaje.flag = 1;
            //printf("%c%d no puede ir a %d\n",memoria_compartida->shared_memory[posLetra], mensaje.grupoActual, grupoDeseado);
        }
        
        if(msgsnd(msgid, &mensaje, sizeof(mensaje) - sizeof(long), 0) == -1) {
            perror("Error al enviar mensaje");
            return 1;
        }

    }

    return 0;
}
