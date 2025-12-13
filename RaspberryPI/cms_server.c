#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#define DEBUG 1

#define SQL_CLN_NAME "CMS_SQL"

#define SAFE_FREE(p)   \
	{                  \
		if (NULL != p) \
		{              \
			free(p);   \
			p = NULL;  \
		}              \
	}

#define BUFFER_MAX_COUNT 128
#define CLIENT_MAX_COUNT 32

#define ID_MAX_COUNT 16
#define PW_MAX_COUNT 16
#define COMMAND_MAX_COUNT 8

typedef struct
{
	int index;
	int fd;
	char ip[16];
	char id[ID_MAX_COUNT];
	char pw[PW_MAX_COUNT];
} CLIENT_INFO;

typedef struct
{
	int fd;
	char *cmd;
	char *from;
	char *to;
	char *msg;
} MESSAGE_INFO;

int client_count = 0;
pthread_mutex_t mutex;

void *client_connection(void *_arg);

void send_message(MESSAGE_INFO *_message_info, CLIENT_INFO *_first_client_info);
void send_message_back(MESSAGE_INFO *_message_info, CLIENT_INFO *_first_client_info);

int find_char_pos(const char *s, int ch);

void print_log(char *_msg);
void print_error_and_exit(char *_msg);

int main(int argc, char *argv[])
{
	// socket server - variables
	int server_socket;
	int client_socket;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	int client_address_size;
	int socket_option = 1;

	int buffer_length = 0;
	char buffer[BUFFER_MAX_COUNT] = {0};

	int i = 0;
	char *pToken = NULL;
	char *pCommand[COMMAND_MAX_COUNT] = {0};
	pthread_t thread_id[CLIENT_MAX_COUNT] = {0};

	// argument count check
	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	// read client auth info from "idpw.txt"
	FILE *fd_idpw = fopen("idpw.txt", "r");
	if (NULL == fd_idpw)
	{
		perror("fopen(\"idpw.txt\")");
		exit(1);
	}

	CLIENT_INFO *client_info = calloc(CLIENT_MAX_COUNT, sizeof(CLIENT_INFO));
	if (NULL == client_info)
	{
		perror("calloc() - CLIENT_INFO");
		exit(1);
	}

	char id[ID_MAX_COUNT];
	char pw[PW_MAX_COUNT];
	for (i = 0; i < CLIENT_MAX_COUNT; i++)
	{
		client_info[i].fd = -1;

		buffer_length = fscanf(fd_idpw, "%s %s", id, pw);
		if (buffer_length <= 0)
			continue;

		strcpy(client_info[i].id, id);
		strcpy(client_info[i].pw, pw);
	}
	fclose(fd_idpw);

	// socket server - init
	if (pthread_mutex_init(&mutex, NULL))
		print_error_and_exit("mutex init");

	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
		print_error_and_exit("socket()");

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(atoi(argv[1]));

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&socket_option, sizeof(socket_option));
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		print_error_and_exit("bind()");

	if (listen(server_socket, 5) < 0)
		print_error_and_exit("listen()");

	// socket server - accept
	char idpw[ID_MAX_COUNT + PW_MAX_COUNT + 1];
	while (1)
	{
		client_address_size = sizeof(client_address);
		client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);
		if (client_count >= CLIENT_MAX_COUNT)
		{
			printf("Max Client Accepted\n");
			shutdown(client_socket, SHUT_WR);
			continue;
		}
		else if (client_socket < 0)
		{
			perror("accept()");
			continue;
		}

		buffer_length = read(client_socket, idpw, sizeof(idpw));
		idpw[buffer_length] = '\0';

		if (buffer_length > 0)
		{
			i = 0;
			pToken = strtok(idpw, "[|]");
			while (NULL != pToken)
			{
				pCommand[i] = pToken;
				if (++i >= COMMAND_MAX_COUNT)
					break;
				pToken = strtok(NULL, "[|]");
			}

			for (i = 0; i < CLIENT_MAX_COUNT; i++)
			{
				// client auth on
				if (!strcmp(pCommand[0], "LOGIN"))
				{
					if (!strcmp(client_info[i].id, pCommand[1]))
					{
						if (client_info[i].fd != -1)
						{
							// already have client's file discriptor
							sprintf(buffer, "[%s] Already logged!\n", pCommand[1]);
							write(client_socket, buffer, strlen(buffer));
							print_log(buffer);
							shutdown(client_socket, SHUT_WR);

							client_info[i].fd = -1;

							break;
						}
						if (!strcmp(client_info[i].pw, pCommand[2]))
						{
							// auth ok
							strcpy(client_info[i].ip, inet_ntoa(client_address.sin_addr));
							pthread_mutex_lock(&mutex);
							client_info[i].index = i;
							client_info[i].fd = client_socket;
							client_count++;
							pthread_mutex_unlock(&mutex);

							sprintf(buffer, "[%s] New connected! (ip:%s, fd:%d, sockcnt:%d)\n", pCommand[1], inet_ntoa(client_address.sin_addr), client_socket, client_count);
							print_log(buffer);
							write(client_socket, buffer, strlen(buffer));

							pthread_create(thread_id + i, NULL, client_connection, (void *)(client_info + i));
							pthread_detach(thread_id[i]);

							break;
						}
					}
				}
			}
			if (i == CLIENT_MAX_COUNT)
			{
				// no id match
				sprintf(buffer, "[%s] Authentication Error\n", pCommand[1]);
				write(client_socket, buffer, strlen(buffer));
				print_log(buffer);
				shutdown(client_socket, SHUT_WR);

				i = 0;
			}
		}
		else
		{
			shutdown(client_socket, SHUT_WR);
		}
	}

	SAFE_FREE(client_info);

	return 0;
}

void *client_connection(void *_arg)
{
	CLIENT_INFO *client_info = (CLIENT_INFO *)_arg;
	int index = client_info->index;

	int buffer_length = 0;
	char buffer[BUFFER_MAX_COUNT];
	char buffer_to[BUFFER_MAX_COUNT];
	char buffer_log[BUFFER_MAX_COUNT];

	int i = 0;
	char *pToken = NULL;
	char *pCommand[COMMAND_MAX_COUNT] = {0};

	MESSAGE_INFO message_info;
	CLIENT_INFO *first_client_info;

	first_client_info = (CLIENT_INFO *)((void *)client_info - (void *)(sizeof(CLIENT_INFO) * index));
	while (1)
	{
		memset(buffer, 0, sizeof(buffer));

		buffer_length = read(client_info->fd, buffer, sizeof(buffer) - 1);
		if (buffer_length <= 0)
			break;

		buffer[buffer_length] = '\0';
		pToken = strtok(buffer, "[:]");

		i = 0;
		while (NULL != pToken)
		{
			pCommand[i] = pToken;
			if (++i >= COMMAND_MAX_COUNT)
				break;
			pToken = strtok(NULL, "[:]");
		}

		message_info.fd = client_info->fd;
		message_info.cmd = pCommand[0];
		message_info.from = client_info->id;
		message_info.to = pCommand[1];
		message_info.msg = pCommand[2];

		sprintf(buffer_log, "message : [%s:%s->%s] %s", message_info.cmd, message_info.from, message_info.to, message_info.msg);
		print_log(buffer_log);
		send_message(&message_info, first_client_info);
		// send_message_back(&message_info, first_client_info);
	}

	close(client_info->fd);

	sprintf(buffer_log, "Disconnet ID:%s (ip:%s, fd:%d, sockcnt:%d)\n", client_info->id, client_info->ip, client_info->fd, client_count - 1);
	print_log(buffer_log);

	pthread_mutex_lock(&mutex);
	client_count--;
	client_info->fd = -1;
	pthread_mutex_unlock(&mutex);

	return 0;
}

void send_message(MESSAGE_INFO *_message_info, CLIENT_INFO *_first_client_info)
{
	int i = 0;
	char buffer[BUFFER_MAX_COUNT];

	if (!strcmp(_message_info->cmd, "DEBUG"))
	{
		for (i = 0; i < CLIENT_MAX_COUNT; i++)
		{
			if ((_first_client_info + i)->fd != -1)
			{
				if (strcmp(_message_info->to, "ALL") != 0 && strcmp(_message_info->to, (_first_client_info + i)->id) != 0)
					continue;
				sprintf(buffer, "[%s:%s]%s", _message_info->cmd, _message_info->from, _message_info->msg);
				write((_first_client_info + i)->fd, buffer, strlen(buffer));
			}
		}
	}
	else
	{
		for (i = 0; i < CLIENT_MAX_COUNT; i++)
		{
			if ((_first_client_info + i)->fd != -1)
			{
				if (!strcmp(_message_info->to, (_first_client_info + i)->id))
				{
					sprintf(buffer, "[%s:%s]%s", _message_info->cmd, _message_info->from, _message_info->msg);
					write((_first_client_info + i)->fd, buffer, strlen(buffer));
				}
			}
		}
	}
}
void send_message_back(MESSAGE_INFO *_message_info, CLIENT_INFO *_first_client_info)
{
	return;

	char buffer[BUFFER_MAX_COUNT];
	sprintf(buffer, "ECHO : %s", _message_info->msg);
	write(_message_info->fd, buffer, strlen(buffer));
}

int find_char_pos(const char *s, int ch)
{
	const char *p = strchr(s, ch);
	return p ? (int)(p - s) : -1;
}

void print_log(char *_msg)
{
#if DEBUG == 1
#endif
	fputs(_msg, stdout);
}
void print_error_and_exit(char *_msg)
{
	perror(_msg);
	strcat(_msg, " error");
	fputs(_msg, stderr);
	fputc('\n', stderr);
	exit(1);
}