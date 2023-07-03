#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

int count_user = 0;
int users[MAX_USERS];

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

int REQ_ADD(int client_sock)
{
    char sendMsg[BUFSIZE];
    if (count_user < MAX_USERS)
    {
        int user_id = 0;
        for (int i = 0; i < MAX_USERS; i++)
        {
            if (users[i] == 0)
            {
                users[i] = client_sock;
                count_user++;
                user_id = i+1;
                break;
            }
        }
        printf("User %02d added\n", user_id);

        char msg[BUFSIZE - 10];
        sprintf(msg, "User %02d joined the group!", user_id);

        sprintf(sendMsg, "06 %02d %02d %s", user_id, user_id, msg);

        send_broadcast(sendMsg);
    }
    else
    {
        sprintf(sendMsg, "07 00 00 01");
        send_private(sendMsg, client_sock);
        return 0;
    }

    return 1;
}

void REQ_REM(int client_sock, int idSender)
{
    users[idSender - 1] = 0;
    printf("User %02d removed\n", idSender);
    count_user--;

    char sendMsg[BUFSIZE];
    sprintf(sendMsg, "08 %02d 00 01", idSender);
    send_broadcast(sendMsg);
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
            int added = REQ_ADD(client_sock);
            if(added == 0) {
                break;
            }
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
                if (receiver_sock == 0) // USER NOT FOUND
                {
                    printf("User %s not found\n", idReceiver);
                    strcpy(buf, "07 00 00 03");
                }
                else
                {
                    send_private(buf, receiver_sock);
                    // INVERTE E ENVIA PARA O SENDER
                    sprintf(buf, "06 %s %s %s", idReceiver, idSender, msg);
                }
                send_private(buf, client_sock);
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

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
