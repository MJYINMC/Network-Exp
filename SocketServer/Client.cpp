#include "DataExchange.h"

#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include <winsock2.h>
#include <winuser.h>
#include <conio.h>

#define BLINK       "\e[5m"
#define UNDERLINE   "\e[4m"
#define RED         "\e[41m"    
#define REVERSE     "\e[7m"             
#define NONE        "\e[0m"

/*
a)	���ӣ��������ӵ�ָ����ַ�Ͷ˿ڵķ����
b)	�Ͽ����ӣ��Ͽ������˵�����
c)	��ȡʱ��: �������˸�����ǰʱ��
d)	��ȡ���֣��������˸��������������
e)	������б��������˸�����ǰ���ӵ����пͻ�����Ϣ����š�IP��ַ���˿ڵȣ�
f)	����Ϣ���������˰���Ϣת������Ӧ��ŵĿͻ��ˣ��ÿͻ����յ�����ʾ����Ļ��
g)	�˳����Ͽ����Ӳ��˳��ͻ��˳���
*/

const char * menu[6] = {
    "1. Get time\n",
    "2. Get host name\n",
    "3. Get client info\n",
    "4. Send message\n",
    "5. Close connection\n",
    "6. Exit\n"
};

bool connected = false;
bool reply = false;
int func = 0;
pthread_t th;
/* ���ڻ�ȡsocket�������ݵĻ����� */
char buf[1024];

void cls(){
    printf("\e[H\e[2J\e[3J\e[?25l");
}

bool SendRequest(int sClient, int op){
    int ret;
    if(op == SEND_MSG){
        op = GET_INFO;
        ret = send(sClient, (char *)&op, sizeof(op), 0);
        if(ret == SOCKET_ERROR){
            return false;
        };
        Message msg;
        while(true){
            if(reply){
                reply = false;
                break;
            }
        }
        printf("Enter ID to continue: ");
        scanf("%d", &msg.clientID);
        printf("Enter message to continue: ");
        getchar();
        gets(msg.msg);
        op = SEND_MSG;
        ret = send(sClient, (char *)&op, sizeof(op), 0);
        if(ret == SOCKET_ERROR){
            return false;
        }
        ret = send(sClient, (char *)&msg, sizeof(msg), 0);
        if(ret == SOCKET_ERROR){
            return false;
        }
    }else{
        ret = send(sClient, (char *)&op, sizeof(op), 0);
        if(ret == SOCKET_ERROR){
            return false;
        }
    };
    return true;
}

void * Receive(void * arg){
    SOCKET sClient = *(SOCKET *)arg;
    int ret;
    while(true){
        /*
         recv����������ʵ��copy���ֽ��������recv��copyʱ����
         ��ô������SOCKET_ERROR�����recv�����ڵȴ�Э���������ʱ�����ж��ˣ���ô������0��
         */
        pthread_testcancel();
        ret = recv(sClient, buf, sizeof(buf), 0);
        if(ret > 0){
            if(ret == 4 && *(int *)buf == RECEIVE_MSG){
                int nLeft =  sizeof(Message);
                char * p = buf;
                while (nLeft > 0)
                {
                    // ȷ���������յ��û�Ҫ���͵���Ϣ
                    ret = recv(sClient, p, nLeft, 0);
                    if (ret == 0) {
                        printf("Network error!\n");
                        break;
                    }                    
                    if (ret == SOCKET_ERROR) {
                        continue;
                    }

                    nLeft -= ret;
                    p += ret;
                }
                Message * msg = (Message *) buf;
                printf("Here's a message from client #%d\n%s\n", msg->clientID, msg->msg);
            }else{
                cls();
                puts(buf);
                reply = true;
            }
        }
        if(ret == 0){
            printf("Connection terminated!\n");
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
    COORD coord = {0,0};
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    
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
    SOCKET sClient; //�����׽���
    struct sockaddr_in saServer; //��ַ��Ϣ

    sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return 0;
	}
    /*
    *��������������������������������������������������������������������������������������������������������������������������������*
    |                     No need to take care                       |
    *________________________________________________________________*
    */
    
    /* ��ȡ����hostname */
    char hostname[MAXBYTE];
    gethostname(hostname, MAXBYTE);

    char localIP[50];
    int localPORT = 0;
	char serverIP[50];
    int serverPORT;
    /* 
    �����ֶ�����IP�Ͷ˿ںţ�Ҳ����ʹ�������в��� 
    ��ʽ��: .\Client.exe 127.0.0.1 8080
    */
    if(argc == 3) {
        strcpy(serverIP, argv[1]);
        serverPORT = atoi(argv[2]);
        goto Start_connecting;
    }
    /* �������� */
    Connect:
    while(!connected){
        printf("Server IP: ");
        scanf("%s", serverIP);
        printf("Server Port: ");
        scanf("%d", &serverPORT);
        Start_connecting:
        cls();
        saServer.sin_family = AF_INET; //��ַ����
	    saServer.sin_port = htons(serverPORT); //ע��ת��Ϊ�����ֽ���
	    saServer.sin_addr.S_un.S_addr = inet_addr(serverIP);
        if (connect(sClient, (struct sockaddr *)&saServer, sizeof(saServer)) == SOCKET_ERROR)
        {
            printf("Fail to connect!\nCheck IP and Port and try again\n");
            closesocket(sClient);
            sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }else{
            /* ����û��bind, ʹ��getsockname��ȡIP�Ͷ˿ں� */
            struct sockaddr addr;
            int len;
            getsockname(sClient, &addr, &len);
            unsigned char * p = (unsigned char *)addr.sa_data;
            sprintf(localIP, "%d.%d.%d.%d", p[2], p[3], p[4], p[5]);
            *(unsigned char *)(&localPORT) = p[1];
            *((unsigned char *)(&localPORT) + 1) = p[0];
            /* �����߳�ִ��recv */
            unsigned long ul = 1;
            ioctlsocket(sClient, FIONBIO, (unsigned long *)&ul);
            pthread_create(&th, NULL, Receive, &sClient);
            // pthread_detach(th);
            connected = true;

        }
    }

    /* ������ѡ�� */
    while(1){
        SetConsoleCursorPosition(hOutput, coord); // ���ƿ���̨�������
        printf("%s @ %s:%d\n", hostname, localIP, localPORT);
        printf(BLINK RED "Connected to %s:%d\n" NONE, serverIP, serverPORT);
        for(int i = 0; i < 6; i++){
            if(i == func){
                printf(REVERSE "%s" NONE, menu[i]);
            }else{
                printf(menu[i]);
            }
        }
        printf(UNDERLINE "(Use Tab or Up/Down Arrow to switch between menu items)\n" NONE);
        printf(UNDERLINE "(You can also type a number to choose menu item directly)\n" NONE);
        Get_input:
        switch (_getch())
        {
            case VK_TAB:
                func = (func+1) % 6;
                break;

            case  VK_RETURN:
                /* ������ĳһ��� */
                exe_begin:
                switch (func)
                {
                    case CLOSE:
                        pthread_cancel(th);
                        pthread_join(th, NULL);
                        closesocket(sClient);
                        connected = false;
                        func = 0;
                        printf("\e[H\e[2J\e[3JDisconnecting from %s:%d!\n", serverIP, serverPORT);
                        sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        /* ����һ���µ����� */
                        goto Connect;
                        break;

                    case EXIT:
                        goto Exit;
                        break;

                    default:
                        if(SendRequest(sClient, func)){
                            while(true){
                                if(reply){
                                    reply = false;
                                    system("pause");
                                    cls();  
                                    break;    
                                }
                            }

                        };
                        break;                     
                }
                break;

            case VK_MODECHANGE & 'C': // Ctrl+C
                goto Exit;

            case 0xE0:
                switch (_getch())
                {
                    case 0x48: // �Ϸ����
                        func = ((func-1 < 0) ? func + 5 : func -1) % 6;
                        break;
                    case 0x50: // �·����
                        func = (func+1) % 6;
                        break;
                    default:
                        goto Get_input; 
                        break;
                }
                break;
            case '1':
                func = 0;
                goto exe_begin;
                break;
            case '2':
                func = 1;
                goto exe_begin;
                break;
            case '3':
                func = 2;
                goto exe_begin;
                break;
            case '4':
                func = 3;
                goto exe_begin;
                break;
            case '5':
                func = 4;
                goto exe_begin;
                break;
            case '6':
                func = 5;
                goto exe_begin;
                break;
            default:
                goto Get_input;
                break;
        }
    }
    
    /* �˳��ͻ��� */
    Exit:
//    pthread_cancel(th);
//    closesocket(sClient);
    // shutdown(sClient, SD_BOTH);
    pthread_cancel(th);
    pthread_join(th, NULL);
    closesocket(sClient);
	WSACleanup();
    puts("Good bye\n");
    return 0;
}