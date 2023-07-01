all:
	gcc -Wall -c common.c
	gcc -Wall -pthread client.c common.o -o client
	gcc -Wall -pthread server.c common.o -o server
