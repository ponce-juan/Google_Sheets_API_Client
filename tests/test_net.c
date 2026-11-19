/*
 * test_net.c - Tests del módulo de red (T3), sin salir a internet.
 * Se ejecutan con: make test
 *
 * Usan un servidor local (127.0.0.1) levantado por el propio test.
 */

#include "net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "FALLO %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

/* Servidor local en un puerto libre que elige el sistema. */
static int start_listener(int backlog, char port_str[8])
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(fd >= 0);

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    CHECK(bind(fd, (struct sockaddr *)&addr, sizeof addr) == 0);
    CHECK(listen(fd, backlog) == 0);

    socklen_t len = sizeof addr;
    CHECK(getsockname(fd, (struct sockaddr *)&addr, &len) == 0);
    snprintf(port_str, 8, "%u", (unsigned)ntohs(addr.sin_port));
    return fd;
}

static void read_exact(int fd, char *buf, size_t n)
{
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, buf + got, n - got);
        CHECK(r > 0);
        got += (size_t)r;
    }
}

static void test_conecta_y_transmite(void)
{
    char port[8];
    int srv = start_listener(5, port);

    net_info_t info;
    int fd = net_tcp_connect("127.0.0.1", port, 5, &info);
    CHECK(fd >= 0);
    CHECK(strcmp(info.ip, "127.0.0.1") == 0);
    CHECK(info.error[0] == '\0');

    int peer = accept(srv, NULL, NULL);
    CHECK(peer >= 0);

    char buf[5] = {0};
    CHECK(write(fd, "ping", 4) == 4);
    read_exact(peer, buf, 4);
    CHECK(strcmp(buf, "ping") == 0);

    CHECK(write(peer, "pong", 4) == 4);
    memset(buf, 0, sizeof buf);
    read_exact(fd, buf, 4);
    CHECK(strcmp(buf, "pong") == 0);

    net_close(fd);
    net_close(peer);
    net_close(srv);
    printf("ok  - conecta y transmite datos en ambos sentidos\n");
}

static void test_puerto_cerrado(void)
{
    char port[8];
    int srv = start_listener(5, port);
    close(srv); /* ahora nadie escucha en ese puerto */

    net_info_t info;
    int fd = net_tcp_connect("127.0.0.1", port, 5, &info);
    CHECK(fd == -1);
    CHECK(strstr(info.error, "conexión TCP") != NULL);
    CHECK(info.ip[0] == '\0');
    printf("ok  - puerto cerrado: falla con mensaje (%s)\n", info.error);
}

static void test_host_inexistente(void)
{
    net_info_t info;
    /* ".invalid" está reservado: nunca resuelve */
    int fd = net_tcp_connect("host-que-no-existe.invalid", "443", 5, &info);
    CHECK(fd == -1);
    CHECK(strstr(info.error, "resolución DNS") != NULL);
    printf("ok  - host inexistente: falla en el DNS (%s)\n", info.error);
}

static void test_solo_ipv4(void)
{
    net_info_t info;
    int fd = net_tcp_connect("::1", "443", 5, &info); /* dirección IPv6 */
    CHECK(fd == -1);
    CHECK(strstr(info.error, "resolución DNS") != NULL);
    printf("ok  - rechaza IPv6: el cliente solo usa IPv4 (%s)\n", info.error);
}

static void test_argumentos_nulos(void)
{
    net_info_t info;
    CHECK(net_tcp_connect(NULL, "443", 5, &info) == -1);
    CHECK(net_tcp_connect("127.0.0.1", NULL, 5, &info) == -1);
    CHECK(net_tcp_connect(NULL, NULL, 5, NULL) == -1); /* info NULL permitido */
    net_close(-1);                                     /* no debe romper */
    printf("ok  - argumentos nulos y descriptor inválido\n");
}

/*
 * Timeout: un servidor con la cola de conexiones llena no atiende más
 * conexiones. Se abren conexiones hasta que una se queda esperando y el
 * timeout (1 segundo) la corta.
 */
static void test_timeout(void)
{
    char port[8];
    int srv = start_listener(0, port); /* cola mínima y nunca hace accept() */

    int fds[8];
    int n = 0;
    int timed_out = 0;
    time_t t0 = time(NULL);

    for (; n < 8; n++) {
        net_info_t info;
        fds[n] = net_tcp_connect("127.0.0.1", port, 1, &info);
        if (fds[n] < 0) {
            CHECK(strstr(info.error, strerror(ETIMEDOUT)) != NULL);
            timed_out = 1;
            break;
        }
    }
    CHECK(timed_out);
    CHECK(time(NULL) - t0 <= 4); /* cortó por el timeout, no se colgó */

    for (int i = 0; i < n; i++)
        net_close(fds[i]);
    net_close(srv);
    printf("ok  - timeout: la conexión se corta a tiempo\n");
}

int main(void)
{
    test_conecta_y_transmite();
    test_puerto_cerrado();
    test_host_inexistente();
    test_solo_ipv4();
    test_argumentos_nulos();
    test_timeout();
    printf("\nTodos los tests de red pasaron.\n");
    return 0;
}