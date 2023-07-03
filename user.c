#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

int s;
int myId = 0;
int users[MAX_USERS];

void help(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int tratamentoMensagem(char *buf)
{
    char *idMsg = NULL;
    char *idSender = NULL;
    char *idReceiver = NULL;
    char *msg = NULL;

    desmontaMensagem(buf, &idMsg, &idSender, &idReceiver, &msg);
    int id = atoi(idMsg);
    int receiver = atoi(idReceiver);
    int sender = atoi(idSender);
    if (id == 6)
    {
        // MENSAGEM DE ADIÇÃO
        if (sender == receiver)
        {
            // MEU CLIENT FOI ADICIONADO
            if (myId == 0)
            {
                myId = sender;
            }
            users[sender - 1] = sender;
        }
        else // MENSAGEM
        { 
            time_t tempo;
            struct tm *horario;

            time(&tempo);
            horario = localtime(&tempo);
            if (receiver == 0) // BROADCAST
            {
                char cpyMsg[BUFSIZE];
                strcpy(cpyMsg, msg);
                sprintf(msg, "[%02d:%02d] -> all: %s", horario->tm_hour, horario->tm_min, cpyMsg);
            }
            else // PRIVADA
            {
                char cpyMsg[BUFSIZE];
                strcpy(cpyMsg, msg);
                sprintf(msg, "P [%02d:%02d] -> %02d: %s", horario->tm_hour, horario->tm_min, sender, cpyMsg);
            }
        }
        puts(msg);
    }
    else if(id == 4) // LIST USERS
    {
        // CASO SO O MEU CLIENT ESTA CONECTADO, PRINTA MEU ID
        if(msg == NULL)
        { 
            printf("%02d\n", myId);
        } 
        else // CASO EXISTAM OUTROS CLIENTS, PRINTA A LISTA RETORNADA 
        { 
            puts(msg);
        }
    }
    else if (id == 7) // ERROR
    {
        int error = atoi(msg);
        if (error == 1)
        {
            puts("User limit exceeded");
            return 1;
        }
        else if (error == 3)
        {
            puts("Receiver not found");
        }
    }
    else if (id == 8) // OK
    {
        if (myId == sender) // MEU CLIENT FOI DESCONECTADO
        {
            puts("Removed Successfully");
            return 1;
        }
        else // OUTRO CLIENT SE DESCONECTOU
        {
            printf("User %02d left the group!\n", sender);
        }
    }

    return 0;
}

void *receiveThread(void *arg)
{
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);

    while (recv(s, buf, BUFSIZE, 0) > 0)
    {
        int exitThread = tratamentoMensagem(buf);
        memset(buf, 0, BUFSIZE);

        if (exitThread == 1)
        {
            close(s);
            exit(EXIT_SUCCESS);
        }
    }

    close(s);
    return NULL;
}

void send_msg(char *message)
{
    size_t count = send(s, message, BUFSIZE, 0);
    if (count != BUFSIZE)
    {
        error_exit("send");
    }
}

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

    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);

    // ENVIA A MENSAGEM DE INCLUSAO PARA O SERVIDOR
    strcpy(buf, "01"); // MENSAGEM DE INCLUSAO
    send_msg(buf);

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receiveThread, NULL) != 0)
    {
        error_exit("recv_thread");
    }

    while (1)
    {
        // RECEBE A MENSAGEM DO CLIENT
        fgets(buf, BUFSIZE - 1, stdin);

        if (strncmp(buf, "close connection", 16) == 0)
        {
            char msg[BUFSIZE];
            sprintf(msg, "02 %02d", myId);
            send_msg(msg);
            close(s);
            exit(EXIT_SUCCESS);
        }
        else if (strncmp(buf, "list users", 10) == 0)
        {
            char msg[BUFSIZE];
            sprintf(msg, "04");
            send_msg(msg);
        }
        else if (strncmp(buf, "send all", 8) == 0)
        {
            char mensagem[BUFSIZE - 10];
            char sendmsg[BUFSIZE];
            sscanf(buf, "send all \"%[^\"]\"", mensagem);
            sprintf(sendmsg, "06 %02d 00 %s", myId, mensagem);
            send_msg(sendmsg);
        }
        else if (strncmp(buf, "send to", 7) == 0)
        {
            char idReceiver[3];
            char mensagem[BUFSIZE - 10];
            char sendmsg[BUFSIZE];
            sscanf(buf, "send to %2s \"%[^\"]\"", idReceiver, mensagem);
            if(atoi(idReceiver) != myId) // IMPEDE QUE ENVIA A MSG PARA ELE MESMO
            {
                sprintf(sendmsg, "06 %02d %s %s", myId, idReceiver, mensagem);
                send_msg(sendmsg);
            }
        }
    }

    close(s);

    exit(EXIT_SUCCESS);
}