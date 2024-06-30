#include <stdio.h>
#include <windows.h>
#include "cambios2.h"

int* cambioConcedido;

HINSTANCE dll;
int (*fncambios2)(void);
int (*aQuEGrupo)(int);
void (*pon_error)(char*);
void (*incrementarCuenta)(void);
int (*refrescar)(void);
int (*inicioCambios)(int, HANDLE, char*);
int (*inicioCambiosHijo)(int, HANDLE, char*);
int (*finCambios)(void);

HANDLE sem1;
HANDLE sem2;

HANDLE sem3;
HANDLE sem4;

HANDLE sem5;

long prevCount;

struct MemoriaCompartida {
    char shared_memory[100];
    int comprobar;
} *memoria_compartida;

struct estructuraHijo {
    int i;
    int idPadre;
}estructura[32];

void placeMemory(int hijo) {
    char letra = LETRAS[hijo];
    int valor = -1;
    int i;

    switch (letra) {
    case 'A': case 'B': case 'C': case 'D': case 'a': case 'b': case 'c': case 'd':
        valor = 1;
        for (i = 0; i < 20; i += 2) {
            if (memoria_compartida->shared_memory[i] == ' ' && memoria_compartida->shared_memory[i + 1] == ' ') {
                memoria_compartida->shared_memory[i] = letra;
                memoria_compartida->shared_memory[i + 1] = valor;
                break;
            }
        }
        break;
    case 'E': case 'F': case 'G': case 'H': case 'e': case 'f': case 'g': case 'h':
        valor = 2;
        for (i = 20; i < 40; i += 2) {
            if (memoria_compartida->shared_memory[i] == ' ' && memoria_compartida->shared_memory[i + 1] == ' ') {
                memoria_compartida->shared_memory[i] = letra;
                memoria_compartida->shared_memory[i + 1] = valor;
                break;
            }
        }
        break;
    case 'I': case 'J': case 'L': case 'M': case 'i': case 'j': case 'l': case 'm':
        valor = 3;
        for (i = 40; i < 60; i += 2) {
            if (memoria_compartida->shared_memory[i] == ' ' && memoria_compartida->shared_memory[i + 1] == ' ') {
                memoria_compartida->shared_memory[i] = letra;
                memoria_compartida->shared_memory[i + 1] = valor;
                break;
            }
        }
        break;
    case 'N': case 'O': case 'P': case 'R': case 'n': case 'o': case 'p': case 'r':
        valor = 4;
        for (i = 60; i < 80; i += 2) {
            if (memoria_compartida->shared_memory[i] == ' ' && memoria_compartida->shared_memory[i + 1] == ' ') {
                memoria_compartida->shared_memory[i] = letra;
                memoria_compartida->shared_memory[i + 1] = valor;
                break;
            }
        }
        break;
    }
}

int findLetterPos(int i) {
    int j;
    for (j = 0; j < 80; j += 2) {
        if (memoria_compartida->shared_memory[j] == LETRAS[i]) {
            return j;
        }
    }
    return -1;
}

DWORD WINAPI funcHilo(LPVOID param) {
    struct estructuraHijo* datos = (struct estructuraHijo*)param;

    DWORD idHijo = GetCurrentThreadId();
    MSG mensaje;

    PeekMessage(NULL, NULL, WM_USER, WM_USER, PM_REMOVE);

    int hijo = datos->i;
    int idPadre = datos->idPadre;

    int posLetra, grupo, cambio;
    char letra;

    placeMemory(hijo);
    refrescar();
    
    ReleaseSemaphore(sem1, 1, &prevCount);
    WaitForSingleObject(sem2, INFINITE);

    while (memoria_compartida->comprobar) {
        WaitForSingleObject(sem4, INFINITE);
        posLetra = findLetterPos(hijo);
        letra = memoria_compartida->shared_memory[posLetra];
        grupo = memoria_compartida->shared_memory[posLetra + 1];

        cambio = aQuEGrupo(grupo);
        memoria_compartida->shared_memory[posLetra + 1] = cambio;
        
        refrescar();

        int numEnvio = hijo * 100 + grupo;

        // Enviar el mensaje
        if (PostThreadMessage(idPadre, WM_USER, idHijo, numEnvio) == FALSE) {
            printf("ERROR AL ENVIAR MENSAJE HIJO\n");
            return 1;
        }
        
        ReleaseSemaphore(sem3, 1, &prevCount);
        //ESPERO MENSAJE
        if (GetMessage(&mensaje, NULL, WM_USER, WM_USER) == FALSE) {
            printf("ERROR RECEPCION MENSAJE PADRE\n");
            return 1;
        }

        if (mensaje.wParam == 0) {
            //Se puede cambiar
            char letraCambio = memoria_compartida->shared_memory[mensaje.lParam];
            char letraAux = letra;

            memoria_compartida->shared_memory[posLetra] = letraCambio;
            memoria_compartida->shared_memory[posLetra + 1] = grupo;

            memoria_compartida->shared_memory[mensaje.lParam] = letraAux;
            memoria_compartida->shared_memory[mensaje.lParam + 1] = cambio;

            *(cambioConcedido) += 1;
            incrementarCuenta();
        } else {
            //No se puede cambiar
        }

        ReleaseSemaphore(sem4, 1, &prevCount);
    }

    return 0;
}

int cargarDLL() {
    dll = LoadLibrary(TEXT("cambios2.dll"));
    if (dll == NULL) {
        return 1;
    }
    fncambios2 = (int(*)(void)) GetProcAddress(dll, "fncambios2");
    aQuEGrupo = (int(*)(int)) GetProcAddress(dll, "aQuEGrupo");
    pon_error = (void(*)(char*)) GetProcAddress(dll, "pon_error");
    incrementarCuenta = (void(*)(void)) GetProcAddress(dll, "incrementarCuenta");
    refrescar = (int(*)(void)) GetProcAddress(dll, "refrescar");
    inicioCambios = (int(*)(int, HANDLE, char*)) GetProcAddress(dll, "inicioCambios");
    inicioCambiosHijo = (int(*)(int, HANDLE, char*)) GetProcAddress(dll, "inicioCambiosHijo");
    finCambios = (int(*)(void)) GetProcAddress(dll, "finCambios");

    if (fncambios2 == NULL || aQuEGrupo == NULL || pon_error == NULL || incrementarCuenta == NULL || refrescar == NULL || inicioCambios == NULL || inicioCambiosHijo == NULL || finCambios == NULL) {
        return 1;
    }

    return 0;
}

DWORD WINAPI funcHiloPadre(LPVOID param) {
    int retardo = (int)param;
    if (retardo == 0) {
        retardo = 20000;
    } else {
        retardo = 30000;
    }
    Sleep(retardo);
    memoria_compartida->comprobar = 0;
    ReleaseSemaphore(sem5, 1, &prevCount);
    finCambios();
    exit(0);

    return 0;
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

int main(int argc, char* argv[]) {
    // Comprobar parámetros
    if (argc < 2) {
        printf("Modo de uso >> '%s <retardo>'\n", argv[0]);
        return 1;
    }
    if (!isNumber(argv[1])) {
        printf("Error: '%s' no es un n�mero v�lido\n", argv[1]);
        return 1;
    }
    int retardo = atoi(argv[1]);
    if (retardo < 0) {
        printf("Error: El n�mero debe ser mayor o igual a 0\n");
        return 1;
    }

    if (cargarDLL() == -1) {
        printf("Error al cargar DLL\n");
        FreeLibrary(dll);
        return 1;
    }
    

    if (argc < 3) {
        // Crear un objeto de memoria compartida
        HANDLE hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,       // Handle de archivo (INVALID_HANDLE_VALUE para memoria compartida)
            NULL,                       // Atributos de seguridad (predeterminado)
            PAGE_READWRITE,             // Permisos de lectura/escritura
            0,                          // Tamaño máximo alto (para memoria compartida)
            sizeof(MemoriaCompartida),  // Tamaño total de la memoria compartida
            "MiMemoriaCompartida"       // Nombre de la memoria compartida
        );

        if (hMapFile == NULL) {
            printf("ERROR\n");
            return 1;
        }

        // Mapear la memoria compartida al espacio de direcciones del proceso actual
        memoria_compartida = (MemoriaCompartida*)MapViewOfFile(
            hMapFile,                       // Handle del objeto de mapeo de archivos
            FILE_MAP_READ | FILE_MAP_WRITE, // Permisos de acceso (lectura/escritura)
            0,                              // Desplazamiento alto
            0,                              // Desplazamiento bajo
            sizeof(MemoriaCompartida)       // Tamaño de vista a mapear
        );

        if (memoria_compartida == NULL) {
            printf("ERROR\n");
            CloseHandle(hMapFile);
            return 1;
        }

        memoria_compartida->comprobar = 1;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));


        // Obtener el ID del hilo actual
        DWORD threadId = GetCurrentThreadId();

        // Construir la cadena wchar_t con formato correcto
        wchar_t programa[100]; // Tamaño suficiente para contener la cadena resultante
        swprintf(programa, 100, L"cambios2.exe %hs %lu", argv[1], threadId);

        // Convertir wchar_t a char
        char programa_char[100];
        int num_chars = WideCharToMultiByte(CP_ACP, 0, programa, -1, programa_char, 100, NULL, NULL);
        if (num_chars == 0) {
            printf("Error en la conversi�n de WideCharToMultiByte.\n");
            return 1;
        }

        //MUTEX
        HANDLE mutex = CreateMutex(NULL, FALSE, "MutexPadre");
        if (mutex == NULL) {
            return 1;
        }

        // COLA DE MENSAJES
        PeekMessage(NULL, NULL, WM_USER, WM_USER, PM_REMOVE);

        //SEM1
        sem1 = CreateSemaphore(NULL, 0, 1, "sem1");
        if (sem1 == NULL) {
            return 1;
        }
        //SEM2
        sem2 = CreateSemaphore(NULL, 0, 32, "sem2");
        if (sem2 == NULL) {
            return 1;
        }
        //SEM3
        sem3 = CreateSemaphore(NULL, 0, 1, "sem3");
        if (sem3 == NULL) {
            return 1;
        }
        //SEM4
        sem4 = CreateSemaphore(NULL, 1, 1, "sem4");
        if (sem4 == NULL) {
            return 1;
        }

        //SEM5
        sem5 = CreateSemaphore(NULL, 0, 1, "sem5");
        if (sem5 == NULL) {
            return 1;
        }

        inicioCambios(retardo, mutex, memoria_compartida->shared_memory);

        if (!CreateProcess(NULL,                   // Nombre del m�dulo a ejecutar (usamos NULL para usar el nombre del programa)
            programa_char,                         // Nombre del programa a ejecutar
            NULL,                                  // Atributos de seguridad del proceso (por defecto NULL)
            NULL,                                  // Atributos de seguridad del hilo principal (por defecto NULL)
            TRUE,                                  // Manejo de herencia del proceso (por defecto TRUE)
            0,                                     // Flags de creaci�n (por defecto 0)
            NULL,                                  // Bloqueo del entorno del proceso (por defecto NULL)
            NULL,                                  // Directorio de inicio del proceso (por defecto NULL)
            &si,                                   // Informaci�n de inicio del proceso
            &pi)) {                                // Informaci�n sobre el proceso creado
            fprintf(stderr, "Fallo al crear el proceso hijo.\n");
            return 1;
        }

        if (CreateThread(NULL, 0, funcHiloPadre, (LPVOID)retardo, 0, NULL) == NULL) {
            printf("ERROR AL CREAR HILO\n");
            return 1;
        }

        //PRODUCTO CONSUMIDOR
        for (int i = 0; i < 32; i++) {
            WaitForSingleObject(sem1, INFINITE);
        }
        ReleaseSemaphore(sem2, 32, &prevCount);
        MSG mensaje;
        int hijo;
        int grupoActual;
        int grupoDeseado;
        int posLetraCambio;
        int posLetra;

        while (memoria_compartida->comprobar) {
            WaitForSingleObject(sem3, INFINITE);
            //ESPERO MENSAJE
            if (GetMessage(&mensaje, NULL, WM_USER, WM_USER) == FALSE) {
                printf("ERROR RECEPCION MENSAJE PADRE\n");
                return 1;
            }
            hijo = mensaje.lParam / 100;
            grupoActual = mensaje.lParam % 100;
            //COMPRUEBO CAMBIO
            posLetra = findLetterPos(hijo);
            grupoDeseado = memoria_compartida->shared_memory[posLetra + 1];
            if (grupoDeseado == 1) {
                for (int i = 1; i < 16; i += 2) {
                    if (memoria_compartida->shared_memory[i] == grupoActual) {
                        posLetraCambio = i - 1;
                    } else {
                        posLetraCambio = -1;
                    }
                }
            } else if (grupoDeseado == 2) {
                for (int i = 21; i < 36; i += 2) {
                    if (memoria_compartida->shared_memory[i] == grupoActual) {
                        posLetraCambio = i - 1;
                    } else {
                        posLetraCambio = -1;
                    }
                }
            } else if (grupoDeseado == 3) {
                for (int i = 41; i < 56; i += 2) {
                    if (memoria_compartida->shared_memory[i] == grupoActual) {
                        posLetraCambio = i - 1;
                    } else {
                        posLetraCambio = -1;
                    }
                }
            } else if (grupoDeseado == 4) {
                for (int i = 61; i < 76; i += 2) {
                    if (memoria_compartida->shared_memory[i] == grupoActual) {
                        posLetraCambio = i - 1;
                    } else {
                        posLetraCambio = -1;
                    }
                }
            }

            if (posLetraCambio != -1) {
                if (PostThreadMessage(mensaje.wParam, WM_USER, 0, posLetraCambio) == FALSE) {
                    printf("ERROR AL ENVIAR MENSAJE HIJO\n");
                    return 1;
                }
            } else {
                if (PostThreadMessage(mensaje.wParam, WM_USER, 1, posLetraCambio) == FALSE) {
                    printf("ERROR AL ENVIAR MENSAJE HIJO\n");
                    return 1;
                }

            }
        }

        // Esperar a que el proceso hijo termine
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        finCambios();

        // Desmapear la memoria compartida
        UnmapViewOfFile(memoria_compartida);
        // Cerrar el handle del objeto de mapeo de archivos
        CloseHandle(hMapFile);
        CloseHandle(mutex);
        CloseHandle(sem1);
        CloseHandle(sem2);
        CloseHandle(sem3);
        CloseHandle(sem4);
        CloseHandle(sem5);

        //Liberar DLL
        FreeLibrary(dll);
    } else {
        HANDLE memoriaHijo = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, "MiMemoriaCompartida");
        memoria_compartida = (MemoriaCompartida*)MapViewOfFile(
            memoriaHijo,                        // Handle del objeto de mapeo de archivos
            FILE_MAP_READ | FILE_MAP_WRITE,     // Permisos de acceso (lectura/escritura)
            0,                                  // Desplazamiento alto
            0,                                  // Desplazamiento bajo
            sizeof(MemoriaCompartida)           // Tama�o de vista a mapear
        );

        HANDLE mutexHijo = OpenMutex(MUTEX_ALL_ACCESS, TRUE, "MutexPadre");

        inicioCambiosHijo(retardo, mutexHijo, memoria_compartida->shared_memory);

        cambioConcedido = (int*)&(memoria_compartida->shared_memory[84]);
        *(cambioConcedido) = 0;

        sem1 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "sem1");
        if (sem1 == NULL) {
            return 1;
        }
        sem2 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "sem2");
        if (sem2 == NULL) {
            return 1;
        }
        sem3 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "sem3");
        if (sem3 == NULL) {
            return 1;
        }
        sem4 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "sem4");
        if (sem4 == NULL) {
            return 1;
        }

        sem5 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "sem5");
        if (sem5 == NULL) {
            return 1;
        }

        for (int i = 0; i < 32; i++) {
            estructura[i].i = i;
            estructura[i].idPadre = atoi(argv[2]);

            if (CreateThread(NULL, 0, funcHilo, (LPVOID)&estructura[i], 0, NULL) == NULL) {
                printf("ERROR AL CREAR HILO\n");
                return 1;
            }
        }

        WaitForSingleObject(sem5, INFINITE);

        //fncambios2();

        // Desmapear la memoria compartida
        UnmapViewOfFile(memoria_compartida);
        // Cerrar el handle del objeto de mapeo de archivos
        CloseHandle(memoriaHijo);
        CloseHandle(mutexHijo);
        CloseHandle(sem1);
        CloseHandle(sem2);
        CloseHandle(sem3);
        CloseHandle(sem4);
        CloseHandle(sem5);

        //Liberar DLL
        FreeLibrary(dll);
    }
    return 0;
}
