#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define SIZE 1024
#define TIME_PORT 2013
#define MAX_CLIENTS 2
#define CHAT_ROOMS 6

struct Node
{
	int client_sockfd;
	struct Node * next;
};

typedef struct Node Node;


// Devin Wright
// Kyle Bondo
// Jonathan Lewis

Node * makeNode(int client_sockfd);
void addLast(Node ** head, int client_sockfd);
int pop(Node ** head);
void greet(int client_sockfd);
int joinRoom(int curClient, int prevRoom, int prevSpot);
void addToRoom(int client_sockfd, int newRoom, int prevRoom, int prevSpot);
void searchRoomForClient(int curClient, int * x, int * y);
void queueToRoom(int x, int y);

char buf[SIZE];
int 	clientsConnected[CHAT_ROOMS][MAX_CLIENTS],
		population[CHAT_ROOMS];
Node * queue[CHAT_ROOMS];

int main(int argc, char * argv[])
{  
	struct sockaddr_in 	serv_addr, 
				client_addr;

	int 	len, 
		sockfd, 
		client_sockfd, 
		fdmax, 
		i, 
		j,
		x,
		y,
		nread;

	fd_set 	masterfd, 
		cpmasterfd;

	for(x = 0; x < CHAT_ROOMS; x++)
		queue[x] = NULL;

	FD_ZERO(&masterfd);
	FD_ZERO(&cpmasterfd);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{  
		perror(NULL);
		exit(2);
	}


	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TIME_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(sockfd, 5);

	FD_SET(sockfd, &masterfd);

	fdmax = sockfd;

	while(1)
	{
		cpmasterfd = masterfd;

		select(fdmax+1, &cpmasterfd, NULL, NULL, NULL);
		
		for(i = 0; i <= fdmax; i++)
		{
			if(FD_ISSET(i, &cpmasterfd))
			{
				if(i == sockfd) // new Connection
				{
					len = sizeof(client_addr);
					client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&len);

					FD_SET(client_sockfd, &masterfd);

					if(client_sockfd > fdmax)
						fdmax = client_sockfd;

					greet(client_sockfd);
				}
				else // old Client wants to talk
				{
					searchRoomForClient(i, &x, &y);

					nread = read(i, buf, SIZE);
					printf("%d, %s.\n", nread, buf);

					if(nread <= 0 || strcmp(buf, "exit") == 0)
					{
						printf("Client disconnected.\n");

						if(x != -1 && y != -1)
						{
							population[x] -= 1;
							clientsConnected[x][y] = 0;

							queueToRoom(x, y);
						}
						
						if(i == fdmax)
							fdmax--;

						close(i);
						FD_CLR(i, &masterfd);
					}
					else if(joinRoom(i, x, y) != 0) {}
					//else if(commands() != 0) {}
					else
					{
						if(x != -1 && y != -1 && y < MAX_CLIENTS)
						{
							for(j = 0; j < population[x]; j++)
							{
								if(FD_ISSET(clientsConnected[x][j], &masterfd) && clientsConnected[x][j] != sockfd && clientsConnected[x][j] != i)
									write(clientsConnected[x][j], buf, nread);
							}
							printf("client: %s\n", buf);
						}
					}
				}
			}
		}
	}

	return 0;
}

void greet(int client_sockfd)
{
	char * message, num[100];
	int x;

	write(client_sockfd, "Here are the chat rooms available\n", strlen("Here are the chat rooms available\n"));

	for(x = 0; x < CHAT_ROOMS; x++)
	{
		sprintf(num, "\tRoom %d: %d available spots\n", x, (MAX_CLIENTS - population[x]));
		write(client_sockfd, num, strlen(num));
	}

	write(client_sockfd, "\n\nWhen you're ready type in a number corresponding to the room number you want to join (0-5)\n", strlen("\n\nWhen you're ready type in a number corresponding to the room number you want to join (0-5)\n"));
}

int joinRoom(int curClient, int prevRoom, int prevSpot)
{
	if(strcmp(buf, "0") == 0)
	{
		addToRoom(curClient, 0, prevRoom, prevSpot);
		return 1;
	}
	else if(strcmp(buf, "1") == 0)
	{
		addToRoom(curClient, 1, prevRoom, prevSpot);
		return 1;
	}
	else if(strcmp(buf, "2") == 0)
	{
		addToRoom(curClient, 2, prevRoom, prevSpot);
		return 1;
	}
	else if(strcmp(buf, "3") == 0)
	{
		addToRoom(curClient, 3, prevRoom, prevSpot);
		return 1;
	}
	else if(strcmp(buf, "4") == 0)
	{
		addToRoom(curClient, 4, prevRoom, prevSpot);
		return 1;
	}
	else if(strcmp(buf, "5") == 0)
	{
		addToRoom(curClient, 5, prevRoom, prevSpot);
		return 1;
	}
	return 0;
}

void addToRoom(int client_sockfd, int newRoom, int prevRoom, int prevSpot)
{
	int i;

	// if client was in a previous room, take him out and place him into new room
	if(prevRoom != -1 && prevSpot != -1)
	{
		if(prevSpot >= MAX_CLIENTS)
		{
			// the client wants to join a new room but is in a queue for an old room
			// take client out of queue, pop DOESN'T WORK BECAUSE CLIENT MIGHT BE IN THE MIDDLE OF THE QUEUE
			//pop(&queue[prevRoom]);
			
		}
		else
		{
			clientsConnected[prevRoom][prevSpot] = 0;
			population[prevRoom]--;
			queueToRoom(prevRoom, prevSpot);
		}
	}

	if(population[newRoom] == MAX_CLIENTS)
	{
		//put client into queue
		write(client_sockfd, "The room is full.\nEither join another room or wait till someone leaves to be automatically joined.", strlen("The room is full.\nEither join another room or wait till someone leaves to be automatically joined."));
		addLast(&queue[newRoom], client_sockfd);
	}
	else
	{
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			if(clientsConnected[newRoom][i] == 0)
			{
				clientsConnected[newRoom][i] = client_sockfd;
				population[newRoom]++;
				break;
			}
		}
	}
}

void searchRoomForClient(int curClient, int * x, int * y)
{
	int i, j, k;

	*x = -1;
	*y = -1;

	Node * curr;

	for(i = 0; i < CHAT_ROOMS; i++)
	{
		for(j = 0; j < MAX_CLIENTS; j++)
		{
			if(clientsConnected[i][j] == curClient)
			{
				*x = i;
				*y = j;
				printf("Found client in array, %d, %d\n", *x, *y);
				return;
			}
		}

		for(curr = queue[i], k = 0; curr != NULL; curr = curr->next, k++)
		{
			if(curr->client_sockfd == curClient)
			{
				*x = i;
				*y = k + MAX_CLIENTS;
				printf("Found client in queue, %d, %d\n", *x, *y);
				return;
			}
		}
	}
}

//Queue methods

Node * makeNode(int client_sockfd)
{
	Node * newNode = (Node*)malloc(sizeof(Node));
	newNode->client_sockfd = client_sockfd;
	return newNode;
}

void addLast(Node ** head, int client_sockfd)
{
	Node * curr = *(head);
	Node * newNode = makeNode(client_sockfd);

	if(*(head) == NULL)
		*(head) = newNode;
	else
	{
		while(curr->next != NULL)
			curr = curr->next;
		curr->next = newNode;
	}
}

void printList(Node * head)
{
	Node * curr = head;
	if(head == NULL)
		printf("Empty List\n");
	else
	{
		printf("Here's what's in the queue: ");
		while(curr != NULL)
		{
			printf("%d\n", curr->client_sockfd);
			curr = curr->next;
		}
	}
	printf("\n");
}

int pop(Node ** head)
{	
	Node * temp = (*head)->next;
	int head_client_sockfd = (*head)->client_sockfd;

	free(*head);
	(*head) = NULL;
	(*head) = temp;

	return head_client_sockfd;
}

void queueToRoom(int x, int y)
{
	int queue_sockfd;

	if(queue[x] != NULL)
	{
		queue_sockfd = pop(&queue[x]);
		population[x] += 1;
		clientsConnected[x][y] = queue_sockfd;
		write(queue_sockfd, "You have been connected to the room!", strlen("You have been connected to the room!"));
	}
}
