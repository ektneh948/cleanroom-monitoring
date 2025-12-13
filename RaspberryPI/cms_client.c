#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define DEBUG 1

#define SAFE_FREE(p)	\
{if(NULL!=p){free(p);p = NULL;}}

#define BUFFER_MAX_COUNT 128

#define ID_MAX_COUNT 16
#define PW_MAX_COUNT 16
#define COMMAND_MAX_COUNT 8

char name[ID_MAX_COUNT] = "[Defualt]";
char pw[PW_MAX_COUNT] = "PASSWD";
char buffer[BUFFER_MAX_COUNT];

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

int main(int argc, char * argv[])
{
    int sock;
	struct sockaddr_in server_address;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;

	if(argc != 5) {
		printf("Usage : %s <IP> <port> <name> <pw>\n",argv[0]);
		exit(1);
	}

    sprintf(name, "%s",argv[3]);
    sprintf(pw, "%s", argv[4]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");

    memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family=AF_INET;
	server_address.sin_addr.s_addr = inet_addr(argv[1]);
	server_address.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
		error_handling("connect() error");
    
    sprintf(buffer,"[LOGIN]%s|%s",name, pw);
	write(sock, buffer, strlen(buffer));
	pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
	pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

    pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

    close(sock);

    return 0;
}

void * send_msg(void * arg)
{
	int *sock = (int *)arg;
	int str_len;
	int ret;
	fd_set initset, newset;
	struct timeval tv;
	char name_msg[ID_MAX_COUNT + PW_MAX_COUNT + BUFFER_MAX_COUNT + 1];

	FD_ZERO(&initset);
	FD_SET(STDIN_FILENO, &initset);

	fputs("Input a message! [<CMD>:<TO>]msg (Defualt : [DEBUG:ALL])\n",stdout);
	while(1) {
		memset(buffer,0,sizeof(buffer));
		name_msg[0] = '\0';
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		newset = initset;
		ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);
		if(FD_ISSET(STDIN_FILENO, &newset))
		{
			fgets(buffer, BUFFER_MAX_COUNT, stdin);
			if(!strncmp(buffer,"quit\n",5)) {
				*sock = -1;
				return NULL;
			}
			else if(buffer[0] != '[')
			{
				strcat(name_msg,"[DEBUG:ALL]");
				strcat(name_msg,buffer);
			}
			else
				strcpy(name_msg,buffer);
			if(write(*sock, name_msg, strlen(name_msg))<=0)
			{
				*sock = -1;
				return NULL;
			}
		}
		if(ret == 0) 
		{
			if(*sock == -1) 
				return NULL;
		}
	}
}

void * recv_msg(void * arg)
{
	int * sock = (int *)arg;	
	int i;
	char *pToken;
	char *pArray[COMMAND_MAX_COUNT]={0};

	char name_msg[ID_MAX_COUNT + PW_MAX_COUNT + BUFFER_MAX_COUNT + 1];
	int str_len;
	while(1) {
		memset(name_msg,0x0,sizeof(name_msg));
		str_len = read(*sock, name_msg, ID_MAX_COUNT + PW_MAX_COUNT + BUFFER_MAX_COUNT );
		if(str_len <= 0) 
		{
			*sock = -1;
			return NULL;
		}
		name_msg[str_len] = 0;
		fputs(name_msg, stdout);

		/*   	pToken = strtok(name_msg,"[:]");
			i = 0;
			while(pToken != NULL)
			{
			pArray[i] =  pToken;
			if(i++ >= ARR_CNT)
			break;
			pToken = strtok(NULL,"[:]");
			}

		//		printf("id:%s, msg:%s,%s,%s,%s\n",pArray[0],pArray[1],pArray[2],pArray[3],pArray[4]);
		printf("id:%s, msg:%s\n",pArray[0],pArray[1]);
		*/
	}
}

void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}