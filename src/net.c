#include "net.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


static void set_error(net_info_t *info, const char *what, const char *detail)
{
    snprintf(info->error, sizeof info->error, "%s: %s", what, detail);
}

/*
 * connect() con tiempo máximo.
 *
 * Un connect() normal puede quedarse esperando mucho tiempo si el destino no
 * responde. Para evitarlo: ponemos socket en modo no bloqueante, se lanza
 * el connect(), y se espera con poll() hasta timeout_sec segundos a que el
 * socket quede listo para escribir (conexión establecida o fallida).
 */
static int connect_with_timeout(int fd, const struct sockaddr *addr,
                                socklen_t len, int timeout_sec)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;

    if (connect(fd, addr, len) < 0) {
        if (errno != EINPROGRESS)
            return -1;

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLOUT;
        pfd.revents = 0;

        int poll_res;
        do {
            poll_res = poll(&pfd, 1, timeout_sec * 1000);
        } while (poll_res < 0 && errno == EINTR);

        if (poll_res == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        if (poll_res < 0)
            return -1;

        /* poll() avisa que terminó; SO_ERROR dice si terminó bien o mal. */
        int so_error = 0;
        socklen_t so_len = sizeof so_error;
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len) < 0)
            return -1;
        if (so_error != 0) {
            errno = so_error;
            return -1;
        }
    }

    /* Se vuelve al modo bloqueante, que es el que usa el resto del programa. */
    if (fcntl(fd, F_SETFL, flags) < 0)
        return -1;
    return 0;
}

/* Tiempo máximo para cada lectura/escritura posterior (evita colgarse). */
static void set_io_timeouts(int fd, int timeout_sec)
{
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv); //Timeout recepcion
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv); //Timeout envio
}

/* Conexion al servidor */
int net_tcp_connect(const char *host, const char *port, int timeout_sec,
                    net_info_t *info)
{
    net_info_t local;
    if (info == NULL)
        info = &local;
    info->ip[0] = '\0';
    info->error[0] = '\0';

    if (host == NULL || port == NULL) {
        set_error(info, "argumentos", "host o puerto nulo");
        return -1;
    }

    /* DNS: de "sheets.googleapis.com" a una o más direcciones IPv4. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       /* solo IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    struct addrinfo *res = NULL;
    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        set_error(info, "resolución DNS", gai_strerror(rc));
        return -1;
    }

    /* Se prueba cada dirección hasta que una conecte. */
    int fd = -1;
    int last_errno = 0;
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) {
            last_errno = errno;
            continue;
        }

        if (connect_with_timeout(fd, p->ai_addr, p->ai_addrlen,
                                 timeout_sec) == 0) {
            const struct sockaddr_in *sin = (const struct sockaddr_in *)p->ai_addr;
            inet_ntop(AF_INET, &sin->sin_addr, info->ip, sizeof info->ip);
            set_io_timeouts(fd, timeout_sec);
            break;
        }

        last_errno = errno; /* se guarda antes de close(), que puede pisarlo */
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        set_error(info, "conexión TCP",
                  last_errno != 0 ? strerror(last_errno)
                                  : "sin direcciones IPv4 disponibles");
        return -1;
    }
    return fd;
}

void net_close(int fd)
{
    if (fd >= 0)
        close(fd);
}