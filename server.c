#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

#define MAX_USERS 15

int users[MAX_USERS];
int count_user = 0;

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

void desmontaMensagem(const char *string, char **idMsg, char **idSender, char **idReceiver, char **msg)
{
    char *copiaString = strdup(string);
    char *token = strtok(copiaString, " ");

    if (token != NULL)
    {
        *idMsg = strdup(token);
        token = strtok(NULL, " ");

        if (token != NULL)
        {
            *idSender = strdup(token);
            token = strtok(NULL, " ");

            if (token != NULL)
            {
                *idReceiver = strdup(token);
                token = strtok(NULL, "");

                if (token != NULL)
                {
                    *msg = strdup(token);
                }
            }
        }
    }

    free(copiaString);
}

void send_broadcast(char *msg)
{
    for (int i = 0; i < sizeof(users); i++)
    {
        int client_sock = users[i];
        if (client_sock != 0)
        {
            size_t count = send(client_sock, msg, BUFSIZE, 0);
            if (count != BUFSIZE)
            {
                error_exit("send");
            }
        }
    }
}

void send_private(char *msg, int client_sock)
{
    size_t count = send(client_sock, msg, BUFSIZE, 0);
    if (count != BUFSIZE)
    {
        error_exit("send");
    }
}

void REQ_ADD(int client_sock)
{
    char sendMsg[BUFSIZE];
    if (count_user < MAX_USERS)
    {
        users[count_user++] = client_sock;
        printf("User %02d added\n", count_user);

        char msg[BUFSIZE - 10];
        sprintf(msg, "User %02d joined the group!", count_user);

        sprintf(sendMsg, "06 %02d %02d %s", count_user, 0, msg);

        send_broadcast(sendMsg);
    }
    else
    {
        sprintf(sendMsg, "07 00 00 01");
        send_private(sendMsg, client_sock);
    }
}

void REQ_REM(int client_sock, int idSender)
{
    users[idSender] = 0;
    printf("User %02d removed\n", idSender);

    char sendMsg[BUFSIZE];
    sprintf(sendMsg, "08 00 %02d 01", idSender);
    send_private(sendMsg, client_sock);
}

void RES_LIST(int client_sock)
{
    char list[BUFSIZE - 10];
    list[0] = '\0';

    for (int i = 0; i < sizeof(users); i++)
    {
        if (client_sock != users[i] && users[i] > 0)
        {
            char num[3];
            snprintf(num, sizeof(num), "%02d", i + 1);
            strcat(list, num);
            strcat(list, " ");
        }
    }

    char sendMsg[BUFSIZE];
    sprintf(sendMsg, "04 00 00 %s", list);
    send_private(sendMsg, client_sock);
}

void *client_thread(void *data)
{
    struct client_data *cdata = (struct client_data *)data;
    int client_sock = cdata->client_sock;

    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);

    char *idMsg = NULL;
    char *idSender = NULL;
    char *idReceiver = NULL;
    char *msg = NULL;

    while (recv(client_sock, buf, BUFSIZE, 0) > 0)
    {
        desmontaMensagem(buf, &idMsg, &idSender, &idReceiver, &msg);
        int id = atoi(idMsg);
        if (id == 1)
        {
            REQ_ADD(client_sock);
        }
        else if (id == 2)
        {
            REQ_REM(client_sock, atoi(idSender));
            break;
        }
        else if (id == 4)
        {
            RES_LIST(client_sock);
        }
        else if (id == 6)
        {
            if (atoi(idReceiver) == 0)
            {
                send_broadcast(buf);
            }
            else
            {
                int receiver_sock = users[atoi(idReceiver) - 1];
                send_private(buf, receiver_sock);
            }
        }

        memset(buf, 0, BUFSIZE);
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

    memset(users, 0, sizeof(users));

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
        // users[count_user] = client_sock;
        // printf("User %02d added\n", ++count_user);

        // char msg_con[40];
        // snprintf(msg_con, 40, "User %02d joined the group!", count_user);

        // send_broadcast(msg_con, users, 40);

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
