#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#define BUFSIZE 500

struct MSG
{
    char idMsg[10];
    char idSender[10];
    char idReceiver[10];
    char message[470];
};

void error_exit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

char *montaMensagem(char *idMsg, char *idSender, char *idReceiver, char *message)
{
    char *concatMsg = malloc(BUFSIZE * sizeof(char));
    concatMsg[0] = '\0';

    strcat(concatMsg, idMsg);
    strcat(concatMsg, " ");
    strcat(concatMsg, idSender);
    strcat(concatMsg, " ");
    strcat(concatMsg, idReceiver);
    strcat(concatMsg, " ");
    strcat(concatMsg, message);
    strcat(concatMsg, " ");

    return concatMsg;
}

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

int addparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }

    port = htons(port);

    struct in_addr inaddr4; // 32-bit IPv4
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IPv6
    if (inet_pton(AF_INET6, addrstr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrToString(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    char addstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET)
    {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;

        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addstr, INET6_ADDRSTRLEN + 1))
        {
            error_exit("ntop");
        }
        port = ntohs(addr4->sin_port);
    }
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;

        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addstr, INET6_ADDRSTRLEN + 1))
        {
            error_exit("ntop");
        }
        port = ntohs(addr6->sin6_port);
    }
    else
    {
        error_exit("unknown protocol family.");
    }

    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage)
{

    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }

    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4"))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    else if (0 == strcmp(proto, "v6"))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    else
    {
        return -1;
    }
}