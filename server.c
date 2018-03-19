/*
* Created By
* Deepak Srivatsav, 2016030
* Anubhav Chaudhary, 2016013
* IIIT, Delhi
*/


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

#define IP_ADDRESS "127.0.0.1"
#define PORT 4440

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
	int numUsersFor;
	int userlist[1000];
};

/*
* This function broadcasts a message to all the clients
*/

void broadcast(char message[1024], int sendID, int mlength)
{
	int i = 0;
	for ( ; i < total_connections ; ++i)
	{
		if (connections[i] != sendID)
			send(connections[i], message, mlength, 0);
	}
}

/*
* Converts an integer to a string
*/

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

/*
* Handles ctrl C by deleting the shared memory segment and closing all client sockets
*/

void sigint_handler(int sg)
{
	shmdt(data);
	shmctl(shmid, IPC_RMID, NULL);
	printf("handled\n");
	int i = 0;
	for ( ; i < total_connections ; ++i)
	{
		close(connections[i]);
	}
	exit(1);
}

/*
* Updates the shared memory while removing the user
*/

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
	char num[10];
	//printf("%d\n", s2);
	tostring(num, userID);
	char bcast [1048];
	strcpy(bcast, "User ");
	strcat(bcast, num);
	strcat (bcast, " disconnected");
	bcast[18 + strlen(num)] = '\0';
	broadcast(bcast, userID, strlen(bcast));
}

/*
* Each client is a thread, and this function handles the thread
*/

void *handler(void *temp)
{
	int s2 = *(int *)temp;
	int read_size;
	char client_message[2000];
	struct msg m;
	while((read_size = recv(s2, &m, sizeof(m), 0)) > 0)
	{
		
		if (m.userlist[0] == 0)
		{
			broadcast(m.message, m.fromID, strlen(m.message));
		}
		else
		{
			printf("%s\n", "sending message");
			int i = 0;
			for ( ; i < m.numUsersFor ; ++i)
			{
				if (send(m.userlist[i], &m.message, strlen(m.message), 0) == -1)
				{
					perror("send");
					puts("invalid user id sent");
					send(m.fromID, "Invalid user selected\n", 21, 0);
				}
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
	server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	server.sin_port = htons(PORT);

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
		tostring(num, s2);
		send(s2, num, strlen(num), 0);

		char bcast [1048];
		strcpy(bcast, "User ");
		strcat(bcast, num);
		strcat (bcast, " connected");
		bcast[15 + strlen(num)] = '\0';
		broadcast(bcast, s2, strlen(bcast));

		pthread_create(&thread, NULL, handler, (void *)temp);
	}
	shmdt(data);
	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}