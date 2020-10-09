CFLAGS=-D_DEFAULT_SOURCE -std=c11 -pedantic -Wall -Wvla -Werror -g

all: client server maint gstat

client: client.o socket.o utils.o
	cc $(CFLAGS) -o client client.o socket.o utils.o

server: server.o utils.o ipc.o socket.o
	cc $(CFLAGS) -o server server.o utils.o ipc.o socket.o

maint: maint.o utils.o ipc.o
	cc $(CFLAGS) -o maint maint.o utils.o ipc.o

gstat: gstat.o utils.o ipc.o
	cc $(CFLAGS) -o gstat gstat.o utils.o ipc.o

client.o: utils.h client.c
	cc $(CFLAGS) -c client.c

server.o: utils.h progStats.h server.c
	cc $(CFLAGS) -c server.c

maint.o: utils.h progStats.h maint.c
	cc $(CFLAGS) -c maint.c

gstat.o: progStats.h gstat.c
	cc $(CFLAGS) -c gstat.c

utils.o: utils.h utils.c
	cc $(CFLAGS) -c utils.c 

ipc.o: ipc.h ipc.c
	cc $(CFLAGS) -c ipc.c

socket.o: socket.h socket.c
	cc $(CFLAGS) -c socket.c

clean:
	rm -f *.o
	rm -f client
	rm -f server
	rm -f maint
	rm -f gstat
