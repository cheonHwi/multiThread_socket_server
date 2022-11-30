#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h> 
#include <WinSock2.h>
#include <process.h>
#include <string.h> 
#include <time.h>
#include <windows.h>

#pragma comment (lib, "Ws2_32.lib")

int client_init(char* ip, int port);
unsigned int WINAPI do_chat_service(void* param);

struct messageForm
{
    int method;
    char nickname[50];
    char content[MAXBYTE];
} msg;

BOOL roop = 1;
char userInfo[MAXBYTE] = "";

int main(int argc, char* argv[])

{
    char ip_addr[256] = "";
    int port_number = 9999;
    //char nickname[50] = "";
    unsigned int tid;
    int sock;
    char id[MAXBYTE] = "";
    char password[MAXBYTE] = "";
    char* pexit = NULL;
    HANDLE mainthread;


    if (argv[1] != NULL && argv[2] != NULL)
    {
        strcpy(ip_addr, argv[1]);  //서버 주소
        port_number = atoi(argv[2]); //포트 번호
    }

    sock = client_init(ip_addr, port_number);
    if (sock < 0)
    {
        printf("sock_init 에러\n");
        exit(0);
    }



    mainthread = (HANDLE)_beginthreadex(NULL, 0, do_chat_service, (void*)sock, 0, &tid);
    if (mainthread)
    {
        //Sleep(200);
        //while (msg.method == 1)

        while (1)
        {
            gets(msg.content, MAXBYTE);
                pexit = strrchr(msg.content, '/');
                if (pexit)
                    if (strcmp(pexit, "/x") == 0)
                        break;
            send(sock, (struct messageForm*)&msg, sizeof(msg), 0);
        }

        closesocket(sock);
        WSACleanup();
        CloseHandle(mainthread);
    }

    return 0;

}

int client_init(char* ip, int port)
{
    SOCKET server_socket;
    WSADATA wsadata;
    SOCKADDR_IN server_address = { 0 };

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    {
        printf("WSAStartup 에러\n");
        return -1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        puts("socket 에러.");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip);
    server_address.sin_port = htons(port);

    if ((connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address))) < 0)
    {
        puts("connect 에러.");
        return -1;
    }

    return server_socket;
}

unsigned int WINAPI do_chat_service(void* params)
{
    SOCKET s = (SOCKET)params;
    char recv_message[MAXBYTE];
    int len = 0;
    int index = 0;
    WSANETWORKEVENTS ev;
    HANDLE event = WSACreateEvent();

    WSAEventSelect(s, event, FD_READ | FD_CLOSE);
    while (1)
    {
        index = WSAWaitForMultipleEvents(1, &event, FALSE, INFINITE, FALSE);
        if ((index != WSA_WAIT_FAILED) && (index != WSA_WAIT_TIMEOUT))
        {
            WSAEnumNetworkEvents(s, event, &ev);
            if (ev.lNetworkEvents == FD_READ)
            {
                int len = recv(s, (struct messageForm*)&msg, sizeof(msg), 0);
                if (msg.content > 0)
                {
                    if (msg.method == -1)
                        msg.method = atoi(msg.content);
                    else if (!strcmp(msg.nickname, ""))
                        strcpy(msg.nickname, msg.nickname);
                    else
                        printf("%s",msg.content);
                }
            }
            else if (ev.lNetworkEvents == FD_CLOSE)
            {
                printf(" >> 서버 서비스가 중단되었습니다.(종료: \"/x\")\n");
                closesocket(s);
                break;
            }
        }
    }
    WSACleanup();
    _endthreadex(0);

    return 0;
}