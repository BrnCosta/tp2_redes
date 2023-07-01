#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

int s;

void help(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void *receiveThread(void *arg)
{
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);

    while (recv(s, buf, BUFSIZE, 0) > 0)
    {
        printf("%s\n", buf);
        memset(buf, 0, BUFSIZE);
    }

    close(s);
    return NULL;
}

void sendPrivate(char *idMsg, char *idSender, char *idReceiver, char *message)
{
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);

    char *concatMsg = montaMensagem(idMsg, idSender, idReceiver, message);

    size_t count = send(s, concatMsg, BUFSIZE, 0);
    if (count != BUFSIZE)
    {
        error_exit("send");
    }
}

// void sendAll()
// {
// }

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        help(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addparse(argv[1], argv[2], &storage))
    {
        help(argc, argv);
    }

    // INICIA O SOCKET
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        error_exit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // CONECTA COM O SERVIDOR
    if (0 != connect(s, addr, sizeof(storage)))
    {
        error_exit("connect");
    }

    char addstr[BUFSIZE];
    addrToString(addr, addstr, BUFSIZE);

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receiveThread, NULL) != 0)
    {
        error_exit("recv_thread");
    }

    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);

    while (1)
    {
        // RECEBE A MENSAGEM DO CLIENT
        fgets(buf, BUFSIZE - 1, stdin);

        if (strncmp(buf, "close connection", 16) == 0)
        {

            break;
        }
        else if (strncmp(buf, "list users", 10) == 0)
        {
            // listUsers(cdata);
        }
        else if (strncmp(buf, "send all", 8) == 0)
        {
            // sendAll(buf, cdata);
        }
        else if (strncmp(buf, "send to", 7) == 0)
        {
            char idReceiver[3];
            char mensagem[BUFSIZE];
            sscanf(buf, "send to %2s \"%[^\"]\"", idReceiver, mensagem);
            sendPrivate("06", "00", idReceiver, mensagem);
        }

        // ENVIA A MENSAGEM PARA O SERVIDOR
        // SE RETORNO FOR DIFERENTE DO ENVIO = ERROR
        size_t count = send(s, buf, strlen(buf), 0);
        if (count != strlen(buf))
        {
            error_exit("send");
        }
    }

    close(s);

    exit(EXIT_SUCCESS);
}