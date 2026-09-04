/*
 * Problema 2: Productor - Consumidor (banda transportadora con pipes)
 *
 *  - El proceso PADRE es la bodega (productor): pide al usuario 20 valores
 *    entre 1 y 100 (unidades de cada pedido). Una vez ingresado el ultimo,
 *    "cierra el registro" y coloca todos los pedidos en la banda
 *    transportadora, que aqui es un unico pipe.
 *
 *  - Se crean DOS procesos hijos (estacion 1 y estacion 2), los consumidores.
 *    Ambos leen del MISMO extremo de lectura del pipe, por lo que compiten
 *    entre si: el sistema operativo entrega cada pedido a quien llame a
 *    read() primero cuando el pedido este disponible. Ninguno tiene pedidos
 *    fijos asignados, tal como pide el enunciado.
 *
 *  - Cada pedido se envia como un solo entero (sizeof(int) bytes). Como ese
 *    tamano es mucho menor que el limite atomico de un pipe (PIPE_BUF, que
 *    en Linux es de 4096 bytes), cada escritura del padre y cada lectura de
 *    los hijos se comporta como una operacion indivisible: nunca se "parte"
 *    un pedido entre dos lecturas ni se mezcla con otro.
 *
 *  - Cuando el padre termina de escribir los 20 pedidos, cierra su extremo
 *    de escritura. Solo cuando TODAS las copias del extremo de escritura
 *    estan cerradas (la del padre y las que cada hijo debe cerrar tambien),
 *    read() devuelve 0, indicando "fin de archivo" (la banda quedo vacia).
 *    Ahi cada estacion imprime su reporte final.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_PEDIDOS 20

int main() {
    int fd[2]; // fd[0] = lectura (banda -> estaciones), fd[1] = escritura (bodega -> banda)

    if (pipe(fd) == -1) {
        perror("Error al crear la tuberia (banda transportadora)");
        exit(1);
    }

    pid_t pid_estacion1, pid_estacion2;

    // creamos la Estacion 1
    pid_estacion1 = fork();
    if (pid_estacion1 < 0) {
        perror("Error al crear la estacion 1");
        exit(1);
    }

    if (pid_estacion1 == 0) {
        // Esta estacion solo lee de la banda, nunca escribe.
        // Cerrar el extremo de escritura heredado es OBLIGATORIO: si no lo
        // hacemos, el pipe nunca llega a "fin de archivo" para nadie, porque
        // el kernel piensa que todavia podria llegar mas informacion desde
        // este proceso.
        close(fd[1]);

        int pedido, pedidos_procesados = 0, total_unidades = 0;
        ssize_t n;

        while ((n = read(fd[0], &pedido, sizeof(pedido))) > 0) {
            pedidos_procesados++;
            total_unidades += pedido;
            printf("[Estacion 1] tomo un pedido de %d unidades\n", pedido);
        }

        close(fd[0]);

        printf("\n===== Reporte Estacion 1 =====\n");
        printf("Pedidos procesados: %d\n", pedidos_procesados);
        printf("Total de unidades despachadas: %d\n", total_unidades);
        exit(0);
    }

    // creamos la Estacion 2 
    pid_estacion2 = fork();
    if (pid_estacion2 < 0) {
        perror("Error al crear la estacion 2");
        exit(1);
    }

    if (pid_estacion2 == 0) {
        close(fd[1]); // misma razon que en la estacion 1

        int pedido, pedidos_procesados = 0, total_unidades = 0;
        ssize_t n;

        while ((n = read(fd[0], &pedido, sizeof(pedido))) > 0) {
            pedidos_procesados++;
            total_unidades += pedido;
            printf("[Estacion 2] tomo un pedido de %d unidades\n", pedido);
        }

        close(fd[0]);

        printf("\n===== Reporte Estacion 2 =====\n");
        printf("Pedidos procesados: %d\n", pedidos_procesados);
        printf("Total de unidades despachadas: %d\n", total_unidades);
        exit(0);
    }

    // proceso PADRE: bodega (productor)

    // el padre no lee de la banda, solo escribe en ella
    close(fd[0]);

    int pedidos[NUM_PEDIDOS];

    printf("Registro de pedidos del dia (ingrese %d valores entre 1 y 100)\n", NUM_PEDIDOS);
    for (int i = 0; i < NUM_PEDIDOS; i++) {
        int valor;
        do {
            printf("Pedido %d/%d (unidades, 1-100): ", i + 1, NUM_PEDIDOS);
            scanf("%d", &valor);
        } while (valor < 1 || valor > 100);
        pedidos[i] = valor;
    }

    printf("\nRegistro cerrado. Colocando los %d pedidos en la banda transportadora...\n\n",
           NUM_PEDIDOS);

    for (int i = 0; i < NUM_PEDIDOS; i++) {
        write(fd[1], &pedidos[i], sizeof(pedidos[i]));
    }

    // cerramos nuestro extremo de escritura: junto con el cierre que ya
    // hicieron ambas estaciones sobre su copia heredada, esto hace que el
    // pipe quede sin escritores y read() devuelva 0 (fin de la banda)
    close(fd[1]);

    // esperamos a que ambas estaciones terminen e impriman su reporte
    wait(NULL);
    wait(NULL);

    printf("\nTurno finalizado. Ambas estaciones terminaron de procesar la banda.\n");

    return 0;
}