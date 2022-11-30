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

int database_init();
int server_init();
int server_close();
unsigned int WINAPI do_chat_service(void* param);
unsigned int WINAPI recv_and_forward(void* param);
int add_client(int index);
int read_client(int index);
void remove_client(int index);
char* get_client_ip(int index);
int notify_client(char* message);
void select_menu(SOCKET accept_sock, int method, struct messageForm msg);
int send_message(struct messageForm msg);
int load_chat_room(SOCKET accept_sock);
int load_chat_record(SOCKET accept_sock, struct messageForm msg);


typedef struct sock_info
{
	SOCKET s;
	HANDLE ev;
	char ipaddr[50];
} SOCK_INFO;

struct messageForm
{
	int method;
	char nickname[50];
	char content[MAXBYTE];
}msg;


int PORT = 5552;
//const int client_count = 10;
int total_socket_count = 0;
SOCK_INFO sock_array[client_count + 1];

MYSQL* conn, * connection;
MYSQL_RES* result;
MYSQL_ROW row;
char sql[MAXBYTE];

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

	if (database_init() < 0) 
	{
		printf("database initalize error\n");
		exit(0);
	}

	if (server_socket < 0)
	{
		printf("socket server initalize error\n");
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

		sprintf(msg.content, " >> New Client connected(IP : %s)\n", inet_ntoa(addr.sin_addr));
		notify_client(msg.content);
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
	int index = (int)param;
	char message[MAXBYTE];
	SOCKADDR_IN client_address;
	int recv_len = 0, addr_len = 0;
	char* token1 = NULL;
	char* next_token = NULL;

	memset(&client_address, 0, sizeof(client_address));

	if(recv_len = recv(sock_array[index].s, (struct messageForm*)&msg, sizeof(msg), 0)  > 0)
	{
		memset(sql, 0, MAXBYTE);
		memset(message, 0, MAXBYTE);

		int method = msg.method;

		
		if (method == 4)	// 메인 채팅9
		{
			send_message(msg);
		}
		else if (method == 3)	// 채팅방 선택
		{
			load_chat_room(sock_array[index].s);
			load_chat_record(sock_array[index].s, msg);
		}
		else if (method == 1)		// 로그인
		{
			msg.method = 0;
			strcpy(msg.nickname, "test");
			send(sock_array[index].s, (struct messageForm*)&msg, sizeof(msg), 0);
			
		}
		else if (method == 2)	// 회원가입
		{
			printf("회원가입 루프 진입\n");			
		}
		else if (method == 0)
		{
			set_method(sock_array[index].s, msg);
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
	strcpy(msg.content, message);

	for (int i = 1; i < total_socket_count; i++)
		send(sock_array[i].s, (struct messageForm*)&msg, sizeof(msg), 0);

	return 0;
}

int load_chat_room(SOCKET accept_sock)
{
	char sql[MAXBYTE];

	strcpy(sql, "select room_number, room_name from chat_room ");

	mysql_query(conn, sql);
	result = mysql_store_result(conn);

	while (row = mysql_fetch_row(result)) { //값이 없을때까지 변환함
		sprintf(msg.content, "%s\t%s\n", row[0], row[1]);
		msg.method = 4;
		send(accept_sock, (struct messageForm*)&msg, sizeof(msg), 0);
	}
	//mysql_free_result(result);
	return 0;
}

int load_chat_record(SOCKET accept_sock, struct messageForm msg)
{
	SOCKET s = accept_sock;
	char sql[MAXBYTE];
	int room_number = atoi(msg.content);

	sprintf(
		sql,
		"select chat_writer, chat_content, chat_time from (select * from clang.chat_content where chat_room = %d order by chat_index desc limit 10) as a order by chat_index asc;",
		room_number);

	mysql_query(conn, sql);
	result = mysql_store_result(conn);

	while (row = mysql_fetch_row(result)) { //값이 없을때까지 변환함
		sprintf(msg.content, "[%s] %s\n%s", row[0], row[1], row[2]);
		msg.method = 4;
		send(s, (struct messageForm*)&msg, sizeof(msg), 0);
	}
	//mysql_free_result(result);
	return 0;
}

int send_message(struct messageForm msg)
{	
	
	char message[MAXBYTE];
	char timeString[MAXBYTE];

	time_t seconds = time(NULL);
	struct tm* now = localtime(&seconds);

	sprintf(timeString, "%04d/%02d/%02d %02d:%02d:%02d\n", 1900 + now->tm_year, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
	sprintf(message, "[%s] %s\n%s", msg.nickname, msg.content, timeString);

	printf("%s\n", message);

	sprintf(sql, "insert into chat_content(chat_writer, chat_content, chat_time, chat_room) values('%s', '%s', '%s', 1)",
		msg.nickname, msg.content, timeString);

	if (mysql_query(conn, sql))
		printf("%d", mysql_errno(conn));

	notify_client(message);

	return 0;
}

int set_method(SOCKET accept_sock, struct messageForm msg)
{
	msg.method = -1;
	send(accept_sock, (struct messageForm*)&msg, sizeof(msg), 0);

	return 0;
}