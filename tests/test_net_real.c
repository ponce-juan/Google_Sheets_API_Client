/*
 * test_net_real.c - Prueba MANUAL contra internet (T3).
 * Se ejecuta con: make test-red
 *
 * Abre una conexión TCP real a sheets.googleapis.com:443 y muestra a qué IP
 * se conectó. Sirve como evidencia de que el socket funciona de verdad.
 * Opcional: ./build/test_net_real <host> <puerto>
 */

#include "net.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const char *host = argc > 1 ? argv[1] : "sheets.googleapis.com";
    const char *port = argc > 2 ? argv[2] : "443";

    net_info_t info;
    int fd = net_tcp_connect(host, port, 10, &info);
    if (fd < 0) {
        fprintf(stderr, "No se pudo conectar a %s:%s -> %s\n", host, port,
                info.error);
        return 1;
    }

    printf("Conexión TCP establecida con %s:%s (IPv4 %s)\n", host, port,
           info.ip);
    net_close(fd);
    return 0;
}