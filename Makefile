all: client server

client: client.c
	gcc -w -pthread -o client client.c

server: server.c
	gcc -w -pthread -o server server.c

clean: 
	rm server client