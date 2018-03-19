all: client multichat

client: client.c
	gcc -pthread -o client client.c

multichat: multichat.c
	gcc -pthread -o server multichat.c

clean: 
	rm server client