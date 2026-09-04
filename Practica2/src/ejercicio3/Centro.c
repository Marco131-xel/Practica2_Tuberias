/*
 * centro de operaciones
 *
 * Este programa es completamente independiente de sucursal.c: no hay
 * fork() ni relacion padre-hijo entre ellos. Se comunican unicamente a
 * traves de una tuberia con nombre (FIFO), que a diferencia de un pipe
 * anonimo, existe como un archivo especial en el sistema de archivos
 * (se ve con "ls -l"). Por eso cualquier proceso que conozca su ruta puede
 * abrirla, sin importar si tiene parentesco con el proceso que la creo.
 *
 *  1. Si la FIFO no existe, la creamos con mkfifo().
 *  2. La abrimos en modo O_RDWR (lectura Y escritura), aunque el centro
 *     solo va a leer. Este es un truco comun con FIFOs: si abrieramos solo
 *     en modo lectura (O_RDONLY), cada vez que TODAS las sucursales
 *     cerraran su extremo de escritura, read() devolveria 0 (fin de
 *     archivo) y tendriamos que estar reabriendo la FIFO constantemente
 *     para seguir esperando nuevas sucursales. Al mantener nosotros mismos
 *     un extremo de escritura abierto sobre nuestra propia FIFO, el pipe
 *     con nombre nunca se queda sin escritores, asi que read() simplemente
 *     se bloquea esperando el siguiente mensaje, que es justo el
 *     comportamiento de "centro activo todo el dia" que pide el enunciado.
 *  3. Leemos mensajes en un bucle. Cada sucursal envia un mensaje completo
 *     en una sola escritura (por eso cada read() recibe un mensaje
 *     completo, sin necesidad de armar lineas caracter por caracter).
 *  4. Si el mensaje es "REPORTE:nombre:monto", lo registramos con la hora
 *     exacta y acumulamos el total del dia.
 *  5. Si el mensaje es "CERRAR", terminamos el bucle, imprimimos el
 *     resumen del dia y eliminamos la FIFO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define FIFO_NAME "fifo_centro"
#define TAM 256

int main() {
    // creamos la FIFO si todavia no existe (si ya existe de una ejecucion
    // anterior que no la elimino, simplemente la reutilizamos).
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Error al crear la FIFO");
            exit(1);
        }
    }

    printf("Centro de operaciones activo. Esperando reportes de sucursales...\n");
    printf("(Las sucursales pueden conectarse en cualquier momento y en cualquier orden)\n\n");

    int fd = open(FIFO_NAME, O_RDWR);
    if (fd == -1) {
        perror("Error al abrir la FIFO");
        exit(1);
    }

    char buffer[TAM];
    int total_reportes = 0;
    long total_pagos = 0;
    int dia_cerrado = 0;

    while (!dia_cerrado) {
        ssize_t n = read(fd, buffer, TAM - 1);
        if (n <= 0) {
            continue; // no deberia ocurrir gracias al truco O_RDWR
        }
        buffer[n] = '\0';

        if (strncmp(buffer, "CERRAR", 6) == 0) {
            dia_cerrado = 1;

        } else if (strncmp(buffer, "REPORTE:", 8) == 0) {
            char nombre[TAM];
            long monto;

            // formato esperado del mensaje: REPORTE:nombre_sucursal:monto
            if (sscanf(buffer + 8, "%[^:]:%ld", nombre, &monto) == 2) {
                time_t ahora = time(NULL);
                struct tm *t = localtime(&ahora);
                char hora_texto[16];
                strftime(hora_texto, sizeof(hora_texto), "%H:%M:%S", t);

                printf("[%s] Reporte recibido de '%s': Q%ld en pagos procesados\n",
                       hora_texto, nombre, monto);

                total_reportes++;
                total_pagos += monto;
            }
        }
    }

    printf("\n===== RESUMEN DEL DIA =====\n");
    printf("Sucursales que reportaron: %d\n", total_reportes);
    printf("Total de pagos procesados: Q%ld\n", total_pagos);

    close(fd);
    unlink(FIFO_NAME); // eliminamos el archivo de la FIFO al cerrar el dia

    return 0;
}