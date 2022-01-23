#include "DataExchange.h"

#include <stdio.h>
#include <string.h>
#include <list>
#include <iostream>
#include <pthread.h>

#include <winsock2.h>
#include <winuser.h>
#include <conio.h>
#define SERVER_PORT	8080	//侦听端口，端口号只要不和保留端口号冲突，可以随意使用

using namespace std;

char hostname[MAXBYTE];
int count = 0;

typedef struct client{
    int clientID;
    int clientPort;
    char clientIP[50];
}Client;

typedef struct connection
{
    Client client;
    SOCKET sServer;
}Connection;

list<Connection *> cons;


void * RequestHandler(void * arg)
{
    int op;
    char buf[1024] = {0};
    char line[100];
    char DateTime[100] = {0};
    Connection * con = (Connection *)arg;
    printf("Handler for %s:%d is currently working!\n", con->client.clientIP, con->client.clientPort);
    int ret;
    Loop:
    while(true){
        ret = recv(con->sServer, (char*)&op, sizeof(op), 0);
        if(ret > 0){
            if(op == GET_TIME){
                SYSTEMTIME m_time;
                GetLocalTime(&m_time);
                sprintf(DateTime, "%02d-%02d-%02d %02d:%02d:%02d", m_time.wYear, m_time.wMonth,
                m_time.wDay, m_time.wHour, m_time.wMinute, m_time.wSecond);
                send(con->sServer, DateTime, sizeof(DateTime), 0);
                printf("INFO[%d]: ",count++);
                cout << "Time information has been sent to " << con->client.clientIP << ":" << con->client.clientPort << endl;
            }else if(op == GET_HOST){
                send(con->sServer, hostname, MAXBYTE, 0);
                printf("INFO[%d]: ",count++);
                cout << "Host information has been sent to " << con->client.clientIP << ":" << con->client.clientPort << endl;
            }else if(op == GET_INFO){
                sprintf(buf,   
                    "+------+-----------------+------------+\n" 
                    "|  ID  |       IP        |    PORT    |\n"
                    "+------+-----------------+------------+\n");

                for (list<Connection *>::iterator it = cons.begin(); it != cons.end(); ++it) {
                    sprintf(line, "|  %-4d|   %-14s|    %-8d|\n", (*it)->client.clientID, (*it)->client.clientIP, (*it)->client.clientPort);
                    strcat(buf, line);
                }
                strcat(buf, "+------+-----------------+------------+\n");
                send(con->sServer, buf, sizeof(buf), 0);
                printf("INFO[%d]: ",count++);
                cout << "Connection information has been sent to " << con->client.clientIP << ":" << con->client.clientPort << endl;
            }else if(op == SEND_MSG){
                int nLeft =  sizeof(Message);
                char * p = buf;
                while (nLeft > 0)
                {
                    // 确保完整接收到用户要发送的信息
                    ret = recv(con->sServer, p, nLeft, 0);
                    if (ret == 0)
                    {
                        printf("Network error!\n");
                        break;
                    }
                    if (ret == -1) //非阻塞模式下默认返回值是-1
                    {
                        continue;
                    }
                    nLeft -= ret;
                    p += ret;
                }
                Message * msg =(Message * )buf;
                int op = RECEIVE_MSG;
                for (list<Connection *>::iterator it = cons.begin(); it != cons.end(); ++it) {
                    if( (*it)->client.clientID == msg->clientID){
                        msg->clientID = con->client.clientID;
                        if(send((*it)->sServer, (char *)&op, sizeof(op), 0) == SOCKET_ERROR){
                            break;
                        };
                        if( send((*it)->sServer, (char *)msg, sizeof(Message), 0) != SOCKET_ERROR){
                            send(con->sServer, "Message sent successfully!", sizeof("Message sent successfully!"), 0);
                            printf("INFO[%d]: ",count++);
                            printf("A massage from %s:%d has been sent to %s:%d\n",con->client.clientIP, con->client.clientPort, (*it)->client.clientIP, (*it)->client.clientPort);
                            goto Loop;
                        }
                    }
                }
                send(con->sServer, "Error occur!", sizeof("Error occur!"), 0);  
            }
        }
        if(ret == 0){
            printf("%s:%d disconnected!\n", con->client.clientIP, con->client.clientPort);
            closesocket(con->sServer);
            cons.remove(con);
            free(con);
            pthread_exit(NULL);
        };
    };
}

int main(int argc, char const *argv[])
{
    /*
    *————————————————————————————————————————————————————————————————*
    |                     No need to take care                       |
    *________________________________________________________________*
    */    
    WORD wVersionRequested;
	WSADATA wsaData;
    //WinSock初始化：
	wVersionRequested = MAKEWORD(2, 2); //希望使用的WinSock DLL的版本
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		printf("WSAStartup() failed!\n");
		return 0;
	}
	
    //确认WinSock DLL支持版本2.2：
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		printf("Invalid Winsock version!\n");
		return 0;
	}

	//创建socket，使用TCP协议：
    struct sockaddr_in saServer, saClient; //地址信息


    SOCKET sListen, sServer; 	
    //侦听套接字，连接套接字
    //sListen是控制socket, sServer是数据socket
    //多线程编程时，有一个控制socket，多个数据socket
    //使用控制socket调用listen，监听到的数据通过数据socket获取，数据传输完即关闭数据socket
    
    //创建socket，使用TCP协议：
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return 0;
	}
    saServer.sin_family = AF_INET; //地址家族
	saServer.sin_port = htons(SERVER_PORT); //端口号，htons将端口号转为大端机可识别的格式
											//四字节调用htonl，两字节调用htons
											//int传入htons高位截断，不截断反而可能出错
											//htons: Host TO Network Short
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //使用INADDR_ANY指示任意地址,意思是任意IP，只要发送到指定的Host就会被监听
    
    // socket句柄与服务器socket地址绑定：
	//（socket句柄必须与需要进行通信的地址绑定后才能执行功能）
	if (bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer)) == SOCKET_ERROR)
	{
		printf("bind() failed! code:%d\n", WSAGetLastError());
		closesocket(sListen); //关闭套接字
		WSACleanup();
		return 0;
	}

	//侦听连接请求：
	if (listen(sListen, 5) == SOCKET_ERROR)
	{
		printf("listen() failed! code:%d\n", WSAGetLastError());
		closesocket(sListen); //关闭套接字
		WSACleanup();
		return 0;
	}

    /*
    *————————————————————————————————————————————————————————————————*
    |                     No need to take care                       |
    *________________________________________________________________*
    */
    
    /* 获取本机hostname */
    gethostname(hostname, MAXBYTE);
	printf("Currently listening on %d!\n", SERVER_PORT);
    
    int ID = 0;
    //阻塞等待接受客户端连接：
    while(1){
        int length = sizeof(saClient);
        sServer = accept(sListen, (struct sockaddr *)&saClient, &length); //调用完成后，length将被设置为saClient的实际长度
        unsigned long ul = 1;
        ioctlsocket(sServer, FIONBIO, (unsigned long *)&ul);
        if(sServer == INVALID_SOCKET){
            printf("accept() failed! code:%d\n", WSAGetLastError());
            closesocket(sServer); //关闭套接字
            continue;
        }
        pthread_t th;
        Connection * new_con = (Connection *)malloc(sizeof(Connection)) ;
        new_con->client.clientID = ++ID;
        new_con->client.clientPort = ntohs(saClient.sin_port);
        strcpy(new_con->client.clientIP, inet_ntoa(saClient.sin_addr));
        new_con->sServer = sServer;
        if(pthread_create(&th, NULL, RequestHandler, new_con) != 0){
            printf("Faid to create handler!\n");
            free(new_con);
            closesocket(sServer); //关闭套接字
        }else{
            cons.push_back(new_con);
            pthread_detach(th);
        };
    }
    return 0;
}