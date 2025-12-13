#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <mysql/mysql.h>

#define DEBUG 1

#define SAFE_FREE(p)   \
	{                  \
		if (NULL != p) \
		{              \
			free(p);   \
			p = NULL;  \
		}              \
	}

#define BUFFER_MAX_COUNT 128

#define ID_MAX_COUNT 16
#define PW_MAX_COUNT 16
#define COMMAND_MAX_COUNT 8

char name[ID_MAX_COUNT] = "[Defualt]";
char pw[PW_MAX_COUNT] = "PASSWD";
char buffer[BUFFER_MAX_COUNT];

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in server_address;
	pthread_t snd_thread, rcv_thread;
	void *thread_return;

	if (argc != 5)
	{
		printf("Usage : %s <IP> <port> <name> <pw>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "%s", argv[3]);
	sprintf(pw, "%s", argv[4]);

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(argv[1]);
	server_address.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
		error_handling("connect() error");

	sprintf(buffer, "[LOGIN]%s|%s", name, pw);
	write(sock, buffer, strlen(buffer));
	pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
	pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	close(sock);

	return 0;
}

void *send_msg(void *arg)
{
	int *sock = (int *)arg;
	int str_len;
	int ret;
	fd_set initset, newset;
	struct timeval tv;
	char name_msg[ID_MAX_COUNT + PW_MAX_COUNT + BUFFER_MAX_COUNT + 1];

	FD_ZERO(&initset);
	FD_SET(STDIN_FILENO, &initset);

	fputs("Input a message! [<CMD>:<TO>]msg (Defualt : [DEBUG:ALL])\n", stdout);
	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		name_msg[0] = '\0';
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		newset = initset;
		ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);
		if (FD_ISSET(STDIN_FILENO, &newset))
		{
			fgets(buffer, BUFFER_MAX_COUNT, stdin);
			if (!strncmp(buffer, "quit\n", 5))
			{
				*sock = -1;
				return NULL;
			}
			else if (buffer[0] != '[')
			{
				strcat(name_msg, "[DEBUG:ALL]");
				strcat(name_msg, buffer);
			}
			else
				strcpy(name_msg, buffer);
			if (write(*sock, name_msg, strlen(name_msg)) <= 0)
			{
				*sock = -1;
				return NULL;
			}
		}
		if (ret == 0)
		{
			if (*sock == -1)
				return NULL;
		}
	}
}

void *recv_msg(void *arg)
{
	MYSQL *conn;
	MYSQL_ROW sql_row;
	int res;
	char sql_cmd[200] = {0};
	char *host = "localhost";
	char *user = "iot";
	char *pass = "pwiot";
	char *dbname = "iotdb";

	int *sock = (int *)arg;
	int i;
	char *pToken;
	char *pArray[COMMAND_MAX_COUNT] = {0};

	char name_msg[ID_MAX_COUNT + PW_MAX_COUNT + BUFFER_MAX_COUNT + 1];
	int str_len;

	float ptcl;
	float temp;
	float humi;
	int value;
	conn = mysql_init(NULL);

	puts("MYSQL startup");
	if (!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		exit(1);
	}
	else
		printf("Connection Successful!\n\n");

	while (1)
	{
		memset(name_msg, 0x0, sizeof(name_msg));
		str_len = read(*sock, name_msg, ID_MAX_COUNT + PW_MAX_COUNT + BUFFER_MAX_COUNT);
		if (str_len <= 0)
		{
			*sock = -1;
			return NULL;
		}
		// name_msg[str_len] = 0;
		name_msg[strcspn(name_msg, "\n")] = '\0';
		// fputs(name_msg, stdout);

		pToken = strtok(name_msg, "[:|]");
		i = 0;
		while (pToken != NULL)
		{
			pArray[i] = pToken;
			if (++i >= COMMAND_MAX_COUNT)
				break;
			pToken = strtok(NULL, "[:|]");
		}
		// [SENSOR:STM_WF3]12.3|45.6|78.9
		if (!strcmp(pArray[0], "SENSOR") && (i == 5))
		{
			ptcl = atof(pArray[2]);
			temp = atof(pArray[3]);
			humi = atof(pArray[4]);
			sprintf(sql_cmd, "insert into sensor(name, date, time, ptcl, temp, humi) values('%s',now(),now(),%f,%f,%f)", pArray[1], ptcl, temp, humi);
			res = mysql_query(conn, sql_cmd);
			if (!res)
				printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
			else
				fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		}
		// [GETSENSOR:STM_BT3]STM_WF3
		else if (!strcmp(pArray[0], "GETSENSOR") && (i == 3))
		{
			sprintf(sql_cmd, "select ptcl, temp, humi from sensor where name='%s' order by id desc limit 1", pArray[2]);

			res = mysql_query(conn, sql_cmd);
			if (res)
			{
				fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
			}
			else
			{
				MYSQL_RES *sql_result = mysql_store_result(conn);
				if (NULL == sql_result)
				{
					fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
				}
				else
				{
					// int num_fields = mysql_num_fields(sql_result);
					sql_row = mysql_fetch_row(sql_result);

					if (!sql_row)
					{
						if (mysql_errno(conn) != 0)
							fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
						else
							fprintf(stderr, "NOTFOUND\n");

						mysql_free_result(sql_result);
					}
					else
					{
						memset(sql_cmd, 0, sizeof(sql_cmd));
						sprintf(sql_cmd, "[%s:%s]%s|%s|%s\n", pArray[0], pArray[1], sql_row[0], sql_row[1], sql_row[2]);
						write(*sock, sql_cmd, strlen(sql_cmd));

						mysql_free_result(sql_result);
					}
				}
			}
		}
		// [FAN:STM_BT3]80|STM_WF3
		// [FAN:STM_WF3]50|NONE
		// [LED:STM_BT3]1|STM_WF3
		// else if (strstr(pArray[0], "FAN") || strstr(pArray[0], "LED") && (i == 4))
		else if (!strcmp(pArray[0], "FAN") || !strcmp(pArray[0], "LED") && (i == 4))
		{
			value = atoi(pArray[2]);
			sprintf(sql_cmd, "insert into device(name, date, time, value) values('%s', now(), now(), %d) on duplicate key update name = '%s', date = now(), time = now(), value = %d", pArray[0], value, pArray[0], value);
			res = mysql_query(conn, sql_cmd);
			if (!res)
				printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
			else
				fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));

			if (strcmp(pArray[3], "NONE"))
			{
				sprintf(sql_cmd, "[%s:%s]%d|NONE\n", pArray[0], pArray[3], value);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
		}
	}

	mysql_close(conn);
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}