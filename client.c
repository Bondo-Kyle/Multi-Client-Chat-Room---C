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


// Devin Wright
// Kyle Bondo
// Jonathan Lewis


void * read_function(void * sockfd);
void * write_function(void * sockfd);


char buf[SIZE];
int main(int argc, char * argv[])
{  
	pthread_t read_thread, write_thread; 
	int sockfd;
	struct sockaddr_in serv_addr;


	if (argc != 2)
	{   
		fprintf(stderr, "usage: %s IPaddr\n", argv[1]);
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{   
		perror(NULL);
		exit(2);
	}

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(TIME_PORT);


	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{   
		perror(NULL);
		exit(3);
	}

	pthread_create(&write_thread,0,write_function,&sockfd);
	pthread_create(&read_thread,0,read_function,&sockfd);
	pthread_join(write_thread,0);
	pthread_join(read_thread,0);

	return 0;
}


void * read_function(void * sockfd)
{
	int nread;

	do
	{
		nread = read(*(int*)sockfd, buf, SIZE);

		if(nread >= 0)
		{
			write(1, buf, nread);
			printf("\n");
		}
	}while(strcmp(buf, "exit") != 0);

	pthread_exit(0); 
}


void * write_function(void * sockfd)
{
	int len;

	do
	{
		printf("> ");
		gets(buf);
		len = strlen(buf) + 1;
		write(*(int*)sockfd, buf, len);

		if(strcmp(buf, "exit") == 0)
		{
			close(*(int*)sockfd);
			exit(0);
		}

	}while(strcmp(buf, "exit") != 0);
	
	pthread_exit(0); 
}
