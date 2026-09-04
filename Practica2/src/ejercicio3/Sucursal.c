/*
 * terminal de una sucursal
 *
 * Programa independiente de centro.c (no hay fork() entre ellos). Se
 * ejecuta cada vez que una sucursal quiere enviar su reporte del dia, y
 * puede iniciarse en cualquier terminal, en cualquier momento, siempre y
 * cuando el centro ya haya creado la FIFO.
 *
 * Envia uno de dos mensajes por la FIFO:
 *   - "REPORTE:nombre:monto"  -> un reporte normal de cierre de turno
 *   - "CERRAR"                -> senal para que el centro cierre el dia
 *                                 e imprima su resumen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO_NAME "fifo_centro"
#define TAM 256

int main() {
    char nombre[TAM];
    char mensaje[TAM];

    printf("=== Reporte de sucursal ===\n");
    printf("Ingrese el nombre de la sucursal (o escriba 'cerrar' para finalizar el dia): ");
    fgets(nombre, TAM, stdin);
    nombre[strcspn(nombre, "\n")] = '\0'; // quitamos el salto de linea

    if (strcmp(nombre, "cerrar") == 0 || strcmp(nombre, "CERRAR") == 0) {
        strcpy(mensaje, "CERRAR");
    } else {
        long monto;
        printf("Ingrese el total de pagos procesados por %s: ", nombre);
        scanf("%ld", &monto);
        snprintf(mensaje, TAM, "REPORTE:%s:%ld", nombre, monto);
    }

    // Abrimos la FIFO solo para escribir.
    //  - Si el archivo de la FIFO no existe (el centro nunca la creo),
    //    open() falla de inmediato con "No such file or directory".
    //  - Si la FIFO existe pero el centro aun no la ha abierto para
    //    lectura, open() en modo O_WRONLY se queda BLOQUEADO esperando a
    //    que aparezca un lector; esto es parte del comportamiento normal
    //    de sincronizacion de las FIFOs.
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo conectar con el centro de operaciones");
        fprintf(stderr, "(Verifique que centro.c ya este en ejecucion)\n");
        exit(1);
    }

    write(fd, mensaje, strlen(mensaje) + 1);
    close(fd);

    if (strcmp(mensaje, "CERRAR") == 0) {
        printf("Mensaje de cierre enviado al centro de operaciones.\n");
    } else {
        printf("Reporte enviado correctamente al centro de operaciones.\n");
    }

    return 0;
}