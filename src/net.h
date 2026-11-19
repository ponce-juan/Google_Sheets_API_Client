#ifndef NET_H
#define NET_H

/*
 * net.h - Conexión TCP construida a mano con sockets BSD (glibc).
 *
 * T3: solo la conexión TCP. El TLS (T4) se agrega encima de este mismo
 * descriptor de archivo.
 */

#include <netinet/in.h> /* INET_ADDRSTRLEN */

#define NET_ERR_LEN 160

/* Informacion de conexion: IP y error */
typedef struct {
    char ip[INET_ADDRSTRLEN]; /* IPv4 a la que se conectó (vacío si falló) */
    char error[NET_ERR_LEN];  /* mensaje de error (vacío si todo salió bien) */
} net_info_t;

/*
 * Abre una conexión TCP a host:port usando SOLO IPv4.
 *
 * Por qué solo IPv4: la API Key está restringida por IP y esa restricción es
 * sobre IPv4 (ver Documentacion/02-API_SETUP.md). Si la conexión saliera por
 * IPv6, Google bloquearía el pedido con un 403.
 *
 * timeout_sec: segundos máximos para conectar, y también para cada lectura o
 *              escritura posterior sobre el socket.
 * info:        puede ser NULL si no interesa saber la IP ni el detalle del error.
 *
 * Devuelve el descriptor del socket (>= 0), o -1 si falló (el motivo queda
 * en info->error).
 */
int net_tcp_connect(const char *host, const char *port, int timeout_sec,
                    net_info_t *info);

/* Cierra el socket. Ignora descriptores inválidos (< 0). */
void net_close(int fd);

#endif /* NET_H */