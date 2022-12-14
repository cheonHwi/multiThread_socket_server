#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <process.h>
#include <string.h>
#include <mysql.h>
#include <time.h>
#include <windows.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "libmysql.lib")

#define client_count 10

#define HOST "127.0.0.1"
#define USER "root"
#define PASSWORD "1234"
#define DATABASE "clang"

int userLogin();
int database_init();
int loadChatRecord();
int server_init();
int server_close();
unsigned int WINAPI do_chat_service(void* param);
unsigned int WINAPI recv_and_forward(void* param);
int add_client(int index);
int read_client(int index);
void remove_client(int index);
int notify_client(char* message);
char* get_client_ip(int index);

typedef struct sock_info
{
	SOCKET s;
	HANDLE ev;
	char ipaddr[50];
} SOCK_INFO;

int PORT = 5552;
//const int client_count = 10;
SOCK_INFO sock_array[client_count + 1];
int total_socket_count = 0;

MYSQL* conn, * connection;
MYSQL_RES* result;
MYSQL_ROW row;

int main(int argc, char* argv[])
{
	unsigned int tid;				// 스레드 ID
	char message[MAXBYTE];
	HANDLE mainthread;

	if (argv[1] != NULL) {
		PORT = atoi(argv[1]);		// 명령 인수는 문자열이기 때문에 숫자로 변환
	}

	mainthread = (HANDLE)_beginthreadex(NULL, 0, do_chat_service, (void*)0, 0, &tid);
	if (mainthread)
	{
		while (1)
		{
			gets_s(message, MAXBYTE);
			if (strcmp(message, "/x") == 0)
				break;

			notify_client(message);
		}
		server_close();
		WSACleanup();
		CloseHandle(mainthread);
	}

	return 0;
}

int server_init()
{
	WSADATA wsadata;
	SOCKET s;
	SOCKADDR_IN server_address;

	memset(&sock_array, 0, sizeof(sock_array));
	total_socket_count = 0;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		puts("WSAStartup 에러.");
		return -1;
	}

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		puts("socket 에러.");
		return -1;
	}

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(PORT);

	if (bind(s, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		puts("bind 에러");
		return -2;
	}

	if (listen(s, SOMAXCONN) < 0)
	{
		puts("listen 에러");
		return -3;
	}

	return s;
}

int database_init()
{
	conn = mysql_init(NULL);

	if (!mysql_real_connect(conn, HOST, USER, PASSWORD, DATABASE, 3306, NULL, 0))
	{
		printf(" >> database initalize failed.\n");
		printf("%d", mysql_errno(conn));
		mysql_close(conn);
		exit(1);
	}
	else {
		mysql_set_character_set(conn, "euckr");
	}

	return 0;
}

int server_close()
{
	for (int i = 1; i < total_socket_count; i++)
	{
		closesocket(sock_array[i].s);
		WSACloseEvent(sock_array[i].ev);
	}

	return 0;
}

unsigned int WINAPI do_chat_service(void* param)

{
	SOCKET  server_socket;
	WSANETWORKEVENTS ev;
	int index;
	WSAEVENT handle_array[client_count + 1];

	server_socket = server_init();

	if (database_init() < 0) {
		printf("데이터베이스 에러\n");
		exit(0);
	}

	if (server_socket < 0)
	{
		printf("초기화 에러\n");
		exit(0);
	}
	else
	{
		printf(" >> database initalize succeed.");
		printf("\n >> server initalize succeed.(PORT:%d)\n", PORT);

		

		HANDLE event = WSACreateEvent();
		sock_array[total_socket_count].ev = event;
		sock_array[total_socket_count].s = server_socket;
		strcpy(sock_array[total_socket_count].ipaddr, "0.0.0.0");

		WSAEventSelect(server_socket, event, FD_ACCEPT);
		total_socket_count++;

		while (1)
		{
			memset(&handle_array, 0, sizeof(handle_array));
			for (int i = 0; i < total_socket_count; i++)
				handle_array[i] = sock_array[i].ev;

			index = WSAWaitForMultipleEvents(total_socket_count,
				handle_array, FALSE, INFINITE, FALSE);
			if ((index != WSA_WAIT_FAILED) && (index != WSA_WAIT_TIMEOUT))
			{
				WSAEnumNetworkEvents(sock_array[index].s, sock_array[index].ev, &ev);
				if (ev.lNetworkEvents == FD_ACCEPT)
					add_client(index);
				else if (ev.lNetworkEvents == FD_READ)
					read_client(index);
				else if (ev.lNetworkEvents == FD_CLOSE)
					remove_client(index);

			}
		}
		closesocket(server_socket);
	}
	mysql_close(conn);
	WSACleanup();
	_endthreadex(0);

	return 0;

}

int add_client(int index)
{
	SOCKADDR_IN addr;
	int len = 0;
	SOCKET accept_sock;

	if (total_socket_count == FD_SETSIZE)
		return 1;
	else {

		len = sizeof(addr);
		memset(&addr, 0, sizeof(addr));
		accept_sock = accept(sock_array[0].s, (SOCKADDR*)&addr, &len);

		HANDLE event = WSACreateEvent();
		sock_array[total_socket_count].ev = event;
		sock_array[total_socket_count].s = accept_sock;
		strcpy(sock_array[total_socket_count].ipaddr, inet_ntoa(addr.sin_addr));

		WSAEventSelect(accept_sock, event, FD_READ | FD_CLOSE);

		total_socket_count++;
		printf(" >> New Client connected(IP : %s)\n", inet_ntoa(addr.sin_addr));

		char msg[256];
		sprintf(msg, " >> New Client connected(IP : %s)\n", inet_ntoa(addr.sin_addr));
		notify_client(msg);
		//loadChatRecord(accept_sock);
	}

	return 0;
}

int read_client(int index)
{
	unsigned int tid;
	HANDLE mainthread = (HANDLE)_beginthreadex(NULL, 0, recv_and_forward, (void*)index, 0, &tid);
	WaitForSingleObject(mainthread, INFINITE);

	CloseHandle(mainthread);

	return 0;
}

unsigned int WINAPI recv_and_forward(void* param)
{
	char timeString[21];
	int index = (int)param;
	char sql[MAXBYTE];
	char message[MAXBYTE];
	char sended_message[MAXBYTE];
	SOCKADDR_IN client_address;
	int recv_len = 0, addr_len = 0;
	char* token1 = NULL;
	char* next_token = NULL;

	memset(&client_address, 0, sizeof(client_address));

	if ((recv_len = recv(sock_array[index].s, sended_message, MAXBYTE, 0)) > 0)
	{
		addr_len = sizeof(client_address);
		getpeername(sock_array[index].s, (SOCKADDR*)&client_address, &addr_len);


		if (strchr(sended_message, '&'))
		{

			char* content = NULL;
			char* ipaddr = strtok_s(sended_message, "&", &content);

			sprintf(sql, "select * from user_data where user_id='%s' and user_pw='%s'", ipaddr, content);

			mysql_query(conn, sql);

			result = mysql_store_result(conn);

			if(row = mysql_fetch_row(result)) { //값이 없을때까지 변환함
				sprintf(message, "%s&%s", row[1], row[2]);
				send(sock_array[index].s, message, sizeof(TRUE), 0);
			}
			
			mysql_free_result(result);


		}
		else if (strchr(sended_message, '/'))
		{
			char* content = NULL;
			char* ipaddr = strtok_s(sended_message, "/", &content); //공백을 기준으로 문자열 자르기

			time_t seconds = time(NULL);
			struct tm* now = localtime(&seconds);

			sprintf(timeString, "%04d/%02dd/%02d %02d:%02d:%02d\n", 1900 + now->tm_year, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
			sprintf(message, "[%s] %s\n%s", ipaddr, content, timeString);
			sprintf(sql, "insert into chat_content(chat_writer, chat_content, chat_time) values('%s', '%s', '%s')", ipaddr, content, timeString);

			printf("%s", message);
			mysql_query(conn, sql);

			for (int i = 1; i < total_socket_count; i++)
			{
				send(sock_array[i].s, message, MAXBYTE, 0);
			}
		}
	}

	_endthreadex(1);
	return 0;
}

void remove_client(int index)
{
	char remove_ip[256];
	char message[MAXBYTE];

	strcpy(remove_ip, get_client_ip(index));
	printf(" >> Client Disconnected(Index: %d, IP: %s)\n", index, remove_ip);
	sprintf(message, " >> Client Disconnected(IP: %s)\n", remove_ip);

	closesocket(sock_array[index].s);
	WSACloseEvent(sock_array[index].ev);

	total_socket_count--;
	sock_array[index].s = sock_array[total_socket_count].s;
	sock_array[index].ev = sock_array[total_socket_count].ev;
	strcpy(sock_array[index].ipaddr, sock_array[total_socket_count].ipaddr);

	notify_client(message);
}

char* get_client_ip(int index)
{
	static char ipaddress[256];
	int    addr_len;
	struct sockaddr_in    sock;

	addr_len = sizeof(sock);
	if (getpeername(sock_array[index].s, (struct sockaddr*)&sock, &addr_len) < 0)
		return NULL;

	strcpy(ipaddress, inet_ntoa(sock.sin_addr));
	return ipaddress;
}

int notify_client(char* message)
{
	for (int i = 1; i < total_socket_count; i++)
		send(sock_array[i].s, message, MAXBYTE, 0);

	return 0;
}

int loadChatRecord(SOCKET accept_sock)
{
	SOCKET s = accept_sock;

	mysql_query(conn, "select a.* from (select * from chat_content order by chat_index desc limit 10)a order by a.chat_index");
	result = mysql_store_result(conn);

	char chatRecordRow[MAXBYTE];

	while (row = mysql_fetch_row(result)) { //값이 없을때까지 변환함
		sprintf(chatRecordRow,"[%s] %s\n%s", row[1], row[2], row[3]);
		send(s, chatRecordRow, sizeof(chatRecordRow), 0);
	}
	mysql_free_result(result);
}
