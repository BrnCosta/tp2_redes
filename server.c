#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

void help(int argc, char **argv)
{
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data
{
    int client_sock;
    struct sockaddr_storage storage;
};

void send_broadcast(char *msg, int *users, int size_msg)
{
    for (int i = 0; i < sizeof(users); i++)
    {
        int client_sock = users[i];
        if (client_sock != 0)
        {
            size_t count = send(client_sock, msg, size_msg, 0);
            if (count != size_msg)
            {
                error_exit("send");
            }
        }
    }
}

void *client_thread(void *data)
{
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *client_addr = (struct sockaddr *)(&cdata->storage);

    char client_addrstr[BUFSIZ];
    addrToString(client_addr, client_addrstr, BUFSIZ);

    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);
    size_t count = recv(cdata->client_sock, buf, BUFSIZ, 0);
    printf("[msg] %s, %d bytes: %s\n", client_addrstr, (int)count, buf);

    sprintf(buf, "remote endpoint: %.1000s\n", client_addrstr);
    count = send(cdata->client_sock, buf, strlen(buf), 0);
    if (count != strlen(buf))
    {
        error_exit("send");
    }

    close(cdata->client_sock);

    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        help(argc, argv);
    }

    struct sockaddr_storage storage;
    if (server_sockaddr_init(argv[1], argv[2], &storage) != 0)
    {
        help(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        error_exit("socket");
    }

    // CASO O SERVIDOR SEJA FINALIZADO E A PORTA CONTINUE EM USO,
    // ELE IRÁ REUTILIZA-LÁ NA PRÓXIMA EXECUÇÃO
    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        error_exit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage)))
    {
        error_exit("bind");
    }

    if (0 != listen(s, 10))
    {
        error_exit("listen");
    }

    char addstr[BUFSIZ];
    addrToString(addr, addstr, BUFSIZ);
    printf("bound to %s, waiting connections\n", addstr);

    int users[15];
    int count_user;

    for (int i = 0; i < sizeof(users); i++)
    {
        users[i] = 0;
    }

    while (1)
    {
        struct sockaddr_storage client_storage;
        struct sockaddr *client_addr = (struct sockaddr *)(&client_storage);
        socklen_t client_addrlen = sizeof(client_storage);

        int client_sock = accept(s, client_addr, &client_addrlen);
        if (client_sock == -1)
        {
            error_exit("accept");
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata)
            error_exit("malloc");

        cdata->client_sock = client_sock;
        memcpy(&(cdata->storage), &client_storage, sizeof(client_storage));

        // ADICIONO NO ARRAY DE USUARIOS, O ADDRESS DO NOVO USUARIO
        users[count_user] = client_sock;
        printf("User %02d added\n", ++count_user);

        char msg_con[40];
        snprintf(msg_con, 40, "User %02d joined the group!", count_user);

        send_broadcast(msg_con, users, 40);

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
