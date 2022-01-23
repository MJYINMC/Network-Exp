enum {GET_TIME, GET_HOST, GET_INFO, SEND_MSG, CLOSE, EXIT, RECEIVE_MSG};

typedef struct message
{
    int clientID;
    char msg [1000];
}Message;
