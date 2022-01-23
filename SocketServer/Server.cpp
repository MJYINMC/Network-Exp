#include "DataExchange.h"

#include <stdio.h>
#include <string.h>
#include <list>
#include <iostream>
#include <pthread.h>

#include <winsock2.h>
#include <winuser.h>
#include <conio.h>
#define SERVER_PORT	8080	//�����˿ڣ��˿ں�ֻҪ���ͱ����˿ںų�ͻ����������ʹ��

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
                    // ȷ���������յ��û�Ҫ���͵���Ϣ
                    ret = recv(con->sServer, p, nLeft, 0);
                    if (ret == 0)
                    {
                        printf("Network error!\n");
                        break;
                    }
                    if (ret == -1) //������ģʽ��Ĭ�Ϸ���ֵ��-1
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
    *��������������������������������������������������������������������������������������������������������������������������������*
    |                     No need to take care                       |
    *________________________________________________________________*
    */    
    WORD wVersionRequested;
	WSADATA wsaData;
    //WinSock��ʼ����
	wVersionRequested = MAKEWORD(2, 2); //ϣ��ʹ�õ�WinSock DLL�İ汾
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		printf("WSAStartup() failed!\n");
		return 0;
	}
	
    //ȷ��WinSock DLL֧�ְ汾2.2��
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		printf("Invalid Winsock version!\n");
		return 0;
	}

	//����socket��ʹ��TCPЭ�飺
    struct sockaddr_in saServer, saClient; //��ַ��Ϣ


    SOCKET sListen, sServer; 	
    //�����׽��֣������׽���
    //sListen�ǿ���socket, sServer������socket
    //���̱߳��ʱ����һ������socket���������socket
    //ʹ�ÿ���socket����listen��������������ͨ������socket��ȡ�����ݴ����꼴�ر�����socket
    
    //����socket��ʹ��TCPЭ�飺
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return 0;
	}
    saServer.sin_family = AF_INET; //��ַ����
	saServer.sin_port = htons(SERVER_PORT); //�˿ںţ�htons���˿ں�תΪ��˻���ʶ��ĸ�ʽ
											//���ֽڵ���htonl�����ֽڵ���htons
											//int����htons��λ�ضϣ����ضϷ������ܳ���
											//htons: Host TO Network Short
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //ʹ��INADDR_ANYָʾ�����ַ,��˼������IP��ֻҪ���͵�ָ����Host�ͻᱻ����
    
    // socket����������socket��ַ�󶨣�
	//��socket�����������Ҫ����ͨ�ŵĵ�ַ�󶨺����ִ�й��ܣ�
	if (bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer)) == SOCKET_ERROR)
	{
		printf("bind() failed! code:%d\n", WSAGetLastError());
		closesocket(sListen); //�ر��׽���
		WSACleanup();
		return 0;
	}

	//������������
	if (listen(sListen, 5) == SOCKET_ERROR)
	{
		printf("listen() failed! code:%d\n", WSAGetLastError());
		closesocket(sListen); //�ر��׽���
		WSACleanup();
		return 0;
	}

    /*
    *��������������������������������������������������������������������������������������������������������������������������������*
    |                     No need to take care                       |
    *________________________________________________________________*
    */
    
    /* ��ȡ����hostname */
    gethostname(hostname, MAXBYTE);
	printf("Currently listening on %d!\n", SERVER_PORT);
    
    int ID = 0;
    //�����ȴ����ܿͻ������ӣ�
    while(1){
        int length = sizeof(saClient);
        sServer = accept(sListen, (struct sockaddr *)&saClient, &length); //������ɺ�length��������ΪsaClient��ʵ�ʳ���
        unsigned long ul = 1;
        ioctlsocket(sServer, FIONBIO, (unsigned long *)&ul);
        if(sServer == INVALID_SOCKET){
            printf("accept() failed! code:%d\n", WSAGetLastError());
            closesocket(sServer); //�ر��׽���
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
            closesocket(sServer); //�ر��׽���
        }else{
            cons.push_back(new_con);
            pthread_detach(th);
        };
    }
    return 0;
}