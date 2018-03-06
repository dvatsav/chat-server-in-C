#include <sys/socket.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

int connections[1000];
int total_connections = 0;
int map_size = 0;
key_t key;
int shmid;
int *data;

struct msg
{
	char message[2000];
	int toID;
	int fromID;
};


void sigint_handler(int sg)
{
	shmdt(data);
	shmctl(shmid, IPC_RMID, NULL);
	printf("handled\n");
	exit(1);
}

int remove_user(int userID)
{
	int i = 0;
	for ( ; i < total_connections ; ++i)
	{
		if (connections[i] == userID)
		{
			int j = i;
			for ( ; j < total_connections - 1 ; ++j)
			{
				connections[j] = connections[j + 1];
			}
			total_connections--;
			break;
		}
	}
	for ( ; i < total_connections ; ++i)
	{
		printf("%d\n", connections[i]);
		data[i] = connections[i];
	}
}

void broadcast(char message[1024], int sendID, struct msg m)
{
	int i = 0;
	for ( ; i < total_connections ; ++i)
	{
		if (connections[i] != sendID)
			send(connections[i], m.message, strlen(m.message), 0);
	}
}

void *handler(void *temp)
{
	int s2 = *(int *)temp;
	int read_size;
	char client_message[2000];
	struct msg m;
	while((read_size = recv(s2, &m, sizeof(m), 0)) > 0)
	{
		//client_message[read_size] = '\0';
		printf("%s\n", m.message);
		printf("%d\n", m.fromID);
		printf("%d\n", m.toID);
		if (m.toID == 0)
		{
			broadcast(m.message, m.fromID, m);
		}
		else
		{
			printf("%s\n", "sending message");
			if (send(m.toID, &m.message, strlen(m.message), 0) == -1)
			{
				perror("send");
				exit(1);
			}
		}
	}
	 
	if(read_size == 0)
	{
		puts("Client disconnected");
		remove_user(s2);
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}
	free(temp);
	return 0;
}

void tostring(char str[], int num)
{
	int i, rem, len = 0, n;

	n = num;
	while (n != 0)
	{
		len++;
		n /= 10;
	}
	for (i = 0; i < len; i++)
	{
		rem = num % 10;
		num = num / 10;
		str[len - (i + 1)] = rem + '0';
	}
	str[len] = '\0';
}

int main()
{
	if (signal(SIGINT, sigint_handler) == SIG_ERR)
	{
		perror("signal");
		exit(1);
	}
	
	int s1, s2, *temp, *tempId;
	struct sockaddr_in server, client;
	s1 = socket(AF_INET, SOCK_STREAM, 0);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(4440);

	bind(s1, (struct sockaddr *)&server, sizeof(server));
	listen(s1, 3);
	int c;
	c = sizeof(struct sockaddr_in);
	
	/*
		Initialize shared memory segment
	*/

	key = 12345;
	shmid = shmget(key, 2048, 0777 | IPC_CREAT);
	data = (int *)shmat(shmid, (void *)0, 0);

	while (s2 = accept(s1, (struct sockaddr *)&client, (socklen_t*)&c))
	{
		connections[total_connections] = s2;
		total_connections++;
		int i = 0;
		for ( ; i < total_connections ; ++i)
		{
			printf("%d\n", connections[i]);
			data[i] = connections[i];
		}
		pthread_t thread;
		temp = malloc(2);
		tempId = malloc(2);
		*tempId = total_connections;
		*temp = s2;
		char num[10];
		printf("%d\n", s2);
		tostring(num, s2);
		printf("%s\n", num);
		send(s2, num, strlen(num), 0);
		pthread_create(&thread, NULL, handler, (void *)temp);
	}
	shmdt(data);
	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}