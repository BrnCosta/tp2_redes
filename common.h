#pragma once

#include <stdlib.h>
#include <arpa/inet.h>

#define BUFSIZE 500

struct MSG
{
    char idMsg[10];
    char idSender[10];
    char idReceiver[10];
    char message[470];
};

int addparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void addrToString(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

void error_exit(const char *msg);

char *montaMensagem(char *idMsg, char *idSender, char *idReceiver, char *message);