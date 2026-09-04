/*
 * Problema 1: Comunicacion unidireccional y bidireccional con pipes
 *
 *  - Se crean DOS tuberias antes del fork():
 *      pipe_padre_hijo: el padre escribe, el hijo lee
 *      pipe_hijo_padre: el hijo escribe, el padre lee
 *    Se necesitan dos porque un pipe es unidireccional (los datos solo
 *    viajan en un sentido). Si quisieramos que el hijo respondiera por la
 *    misma tuberia que uso el padre, no habria forma de distinguir quien
 *    escribio que, y ademas ambos extremos de lectura/escritura quedarian
 *    mezclados.
 *
 *  - Fase 1 (opcional): verificacion de canal.
 *      El padre pregunta si el cliente quiere verificar el canal.
 *      Si dice que si, escribe una palabra, se la manda al hijo con la
 *      etiqueta "V:" y el hijo la invierte caracter por caracter SIN usar
 *      funciones de libreria para invertir (calculamos la longitud y
 *      copiamos manualmente, sin strrev ni similares).
 *
 *  - Fase 2 (obligatoria): pago.
 *      El padre pide el numero de tarjeta (1000-9999), lo manda al hijo
 *      con la etiqueta "T:" y el hijo responde PAGO_APROBADO (si es par)
 *      o PAGO_RECHAZADO (si es impar). El padre imprime el resultado final.
 *
 *  Usamos un pequeno protocolo de etiquetas ("V:" y "T:") dentro del mismo
 *  pipe_padre_hijo porque ambas fases usan el mismo sentido de comunicacion
 *  (padre -> hijo); lo que cambia es el sentido de la respuesta, que
 *  siempre va por pipe_hijo_padre.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define TAM 100

int main() {
    int pipe_padre_hijo[2]; // [0]=lectura, [1]=escritura padre escribe, hijo lee.
    int pipe_hijo_padre[2]; // [0]=lectura, [1]=escritura hijo escribe, padre lee.
    pid_t pid;
    char opcion;
    char palabra[TAM];

    // creamos ambas tuberias ANTES del fork para que el proceso hijo
    // herede los mismos descriptores de archivo que el padre.
    if (pipe(pipe_padre_hijo) == -1) {
        perror("Error al crear la tuberia padre->hijo");
        exit(1);
    }
    if (pipe(pipe_hijo_padre) == -1) {
        perror("Error al crear la tuberia hijo->padre");
        exit(1);
    }

    pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        exit(1);
    }

    if (pid == 0) {
        /* proceso hijo */

        // el hijo solo necesita: leer de pipe_padre_hijo y escribir en pipe_hijo_padre
        // cerramos los extremos que no usa para no dejar descriptores sueltos
        close(pipe_padre_hijo[1]);
        close(pipe_hijo_padre[0]);

        char buffer[TAM];
        int n;

        // El hijo se queda esperando (bloqueado) mensajes del padre hasta
        // que este cierre su extremo de escritura o se procese el pago.
        while ((n = read(pipe_padre_hijo[0], buffer, TAM - 1)) > 0) {
            buffer[n] = '\0';

            if (strncmp(buffer, "V:", 2) == 0) {
                // verificacion de canal: invertir sin funciones de libreria
                char *texto = buffer + 2;

                int len = 0;
                while (texto[len] != '\0') {
                    len++; // calculamos la longitud manualmente
                }

                char invertido[TAM];
                for (int i = 0; i < len; i++) {
                    invertido[i] = texto[len - 1 - i]; // copiamos en orden inverso
                }
                invertido[len] = '\0';

                write(pipe_hijo_padre[1], invertido, len + 1);

            } else if (strncmp(buffer, "T:", 2) == 0) {
                // validacion del pago
                int numero = atoi(buffer + 2);
                char resultado[TAM];

                if (numero % 2 == 0) {
                    strcpy(resultado, "PAGO_APROBADO");
                } else {
                    strcpy(resultado, "PAGO_RECHAZADO");
                }

                write(pipe_hijo_padre[1], resultado, strlen(resultado) + 1);
                break; // despues del pago, la comunicacion termina
            }
        }

        close(pipe_padre_hijo[0]);
        close(pipe_hijo_padre[1]);
        exit(0);

    } else {
        /* proceso padre*/

        // el padre solo necesita: escribir en pipe_padre_hijo y leer de pipe_hijo_padre.
        close(pipe_padre_hijo[0]);
        close(pipe_hijo_padre[1]);

        char respuesta[TAM];
        int n;

        printf("Desea verificar el estado del canal antes de pagar? (s/n): ");
        scanf(" %c", &opcion);
        getchar(); // limpiamos el salto de linea que queda en el buffer de entrada

        if (opcion == 's' || opcion == 'S') {
            printf("Escriba una palabra para verificar el canal: ");
            fgets(palabra, TAM, stdin);
            palabra[strcspn(palabra, "\n")] = '\0'; // quitamos el salto de linea final

            char mensaje[TAM];
            snprintf(mensaje, TAM, "V:%s", palabra);
            write(pipe_padre_hijo[1], mensaje, strlen(mensaje) + 1);

            n = read(pipe_hijo_padre[0], respuesta, TAM - 1);
            respuesta[n] = '\0';
            printf("Canal activo. El hijo respondio: %s\n", respuesta);
        } else {
            printf("Verificacion omitida. Continuando directo al pago...\n");
        }

        int tarjeta_num;
        do {
            printf("Ingrese el numero de tarjeta (1000-9999): ");
            scanf("%d", &tarjeta_num);
        } while (tarjeta_num < 1000 || tarjeta_num > 9999);

        char mensaje_tarjeta[TAM];
        snprintf(mensaje_tarjeta, TAM, "T:%d", tarjeta_num);
        write(pipe_padre_hijo[1], mensaje_tarjeta, strlen(mensaje_tarjeta) + 1);

        n = read(pipe_hijo_padre[0], respuesta, TAM - 1);
        respuesta[n] = '\0';

        printf("\nResultado final del pago: %s\n", respuesta);

        close(pipe_padre_hijo[1]);
        close(pipe_hijo_padre[0]);

        wait(NULL); // esperamos a que el hijo termine antes de cerrar el padre
    }

    return 0;
}