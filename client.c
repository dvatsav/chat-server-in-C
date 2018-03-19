#include <stdio.h> 
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <poll.h>
#include <pthread.h>

int myId;
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
}m;



int repeat = 0;
int users_available = 0;

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

void sigint_handler(int sg)
{
	exit(1);
}

int str_to_int(char num[1024])
{
	int len = strlen(num);
	int dec = 0, i = 0;
	for(i = 0 ; i < len ; ++i)
	{
		dec = dec * 10 + (num[i] - '0');
	}

	return dec;

}

int printcheck = 0;
void *printprompt(void *temp)
{
	while (1)
	{
		if (printcheck == 1)
		{
			//printf("Select the ID to whom you want to send the message to. Select 0 if you want to send the message to everyone: ");
			printcheck = 0;
		}
	}
}

int check_valid_user(int j)
{
	int *i = data;
	while (*i != NULL)
	{
		if (*i == j)
		{
			return 1;
		}	
		++i;
	}
	return 0;
}

int check_valid_sequence(char nums[])
{
	int i = 0;
	for ( ; i < strlen(nums) ; ++i)
	{
		if ((nums[i] < 48 || nums[i] > 57) && nums[i] != ' ')
		{
			return 5;
		}

	}
	char *result;
	int n;
	result = strtok(nums, " ");
	while (result != NULL)
	{
		n = str_to_int(result);
		if (n != 0 && !check_valid_user(n))
		{

			return 6;
		}	
		result = strtok(NULL, " ");
	}
	return 1;
	
}

void *handlestdin(void *temp)
{
	struct pollfd fds[1];
	int s1 = *(int *)temp;
	int nfds = 1;
	memset(fds, 0 , sizeof(fds));
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	int flag = 0;
	while (1)
	{
		printf("Available users are: \n");
		int total = 0;
		int *i = data;
		while (*i != NULL)
		{
			if (*i != myId)
			{
				printf("%s %d\n", "User ID:", *i);
				total++;
			}	
			++i;
		}	
		
		printf("Select the ID to whom you want to send the message to. Select 0 if you want to send the message to everyone. If you want to send a message to only a group of people, specify the User IDs (space separated): ");
				
		int n;
		char number[1000];
		char *duplicate;

		fgets(number, 1000, stdin);
		number[strlen(number) - 1] = '\0';
		duplicate = strdup(number);
		if (check_valid_sequence(duplicate) == 6)
		{
			puts("Invalid user selected");
			continue;	
		}
		if (check_valid_sequence(duplicate) == 5)
		{
			puts("invalid format of numbers");
			continue;	
		}
		char *result;
		m.numUsersFor = 0;
		result = strtok(number, " ");
		while (result != NULL)
		{
			m.userlist[m.numUsersFor++] = str_to_int(result);	
			result = strtok(NULL, " ");
		}
		//m.toID = n;
		//printf("%d\n", m.toID);
		printf("Enter message that you wish to send : ");
		char message[1000];
		fgets(message, 1024, stdin);
		char id[100];
		tostring(id, m.fromID);
		strcpy(m.message, "\n");
		strcat(m.message, id);
		strcat(m.message, " says: ");
		strcat(m.message, message);
		m.message[strlen(message) + 8] = '\0';

		//Checking error 1, whether a user enters an invalid user id 
		if(send(s1, &m, sizeof(m), 0) < 0)
		{
			puts("Send failed, you have chosen an invalid user id");
			continue;
		}
	}
}

void *handlesocketin(void *temp)
{
	struct pollfd fds[1];
	int nfds = 1;
	int s1 = *(int *)temp;
	memset(fds, 0 , sizeof(fds));
	fds[0].fd = s1;
	fds[0].events = POLLIN;
	int read_size = 0;
	char reply[1000];
	while (1)
	{
		poll(fds, nfds, 10000000);
		if (fds[0].revents)
		{
			if((read_size = recv(s1, reply, sizeof(reply), 0)) < 0)
			{
				puts("recv failed");
				break;
			}
			else
			{
				reply[read_size] = '\0';
				printf("\n%s\n", reply);	
				printcheck = 1;
			}
		}
	}
}


int main(int argc, char **argv)
{
	
	if (signal(SIGINT, sigint_handler) == SIG_ERR)
	{
		perror("signal");
		exit(1);
	}
	
	if (argc < 3)
	{
		printf("usage: ./client <IP Address> <Port>\n");
		return 0;
	}

	char *ip_address = malloc(1024);
	strcpy(ip_address, argv[1]);

	int port = str_to_int(argv[2]);


	int s1;
	struct sockaddr_in server;
	char message[1000], reply[2000];

	if ((s1 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");
	
	
	
	server.sin_addr.s_addr = inet_addr(ip_address);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
 
	if (connect(s1, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}
	puts("Connected\n");
	int read_size;
	if((read_size = recv(s1, reply, sizeof(reply), 0)) < 0)
	{
		puts("recv failed");
		return 1;
	}
	else
	{
		reply[read_size] = '\0';
		printf("%s %s\n","Your User ID is:", reply);
		myId = str_to_int(reply);
	}

	m.fromID = myId;

	printf("%d\n", myId);
	key = 12345;
	shmid = shmget(key, 2048, 0777 | IPC_CREAT);
	data = shmat(shmid, (void *)0, 0); 
	
	
	pthread_t input1, input2, input3;
	int *temp = malloc(2);
	*temp = s1;
	pthread_create(&input1, NULL, handlestdin, (void *)temp);
	pthread_create(&input2, NULL, handlesocketin, (void *)temp);
	//pthread_create(&input3, NULL, printprompt, (void *)temp);
	while (1)
	{
		if (s1 == -1)
			return 0;
	}
	//close(s1);
	//shmdt(data);
	//shmctl(shmid, IPC_RMID, NULL);
	return 0;
}