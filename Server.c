#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>
#define MAX_CLIENTS 30
typedef struct{
    int socket_client;
    char name[10];
} CLIENT;
typedef struct {
    char name[10];
    char msg[1024];
    enum{
        PUBMSG,
        PRIMSG,
        INIT
    } ACTION;
    char pname[10];
} MSG;
MSG tempmsg = { 0 };
CLIENT client[MAX_CLIENTS] = {0};
int master_socket, addrlen, new_client;
/* Thread xử lý việc client gửi tin nhắn */
void *HandlerThread(void *args)
{
    struct sockaddr_in address;
    int max_fd, activity, read_val;
    int fd;
    fd_set readfds;
    struct timeval tv;
    while(1){
        max_fd = 0;
        FD_ZERO(&readfds);

        for(int i=0; i<MAX_CLIENTS; i++){
            fd = client[i].socket_client;

            if(fd > 0){
                FD_SET(fd, &readfds);
            }
            if(fd > max_fd)
                max_fd = fd;
        }

        if(max_fd == 0) continue;
        tv.tv_sec = 3;
        activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if ((activity < 0) && (errno!=EINTR))
		{
			printf("Select error");
            continue;
		}
        for(int i = 0; i< MAX_CLIENTS; i++)
        {
            fd = client[i].socket_client;
            if(FD_ISSET(fd, &readfds))
            {
                memset(&tempmsg, 0 , sizeof(tempmsg));
                if((read_val = read(fd, (void *)&tempmsg, sizeof(tempmsg))) == 0)
                {
                    /* Client disconnected */
                    getpeername(fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    printf("Client disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close( fd );
                    client[i].socket_client = 0;
                    memset(&client[i].name, 0 , sizeof(client[i].name));
                }else{
                    if(tempmsg.ACTION == INIT){
                        /* Init tên người dùng */
                        strcpy(client[i].name, tempmsg.name);
                        break;
                    }
                    if(tempmsg.ACTION == PRIMSG){
                        /* Tin nhắn riêng */
                            for(int j = 0; j< MAX_CLIENTS; j++)
                        {
                            if(strcmp(client[j].name, tempmsg.pname) == 0)
                            {
                                send(client[j].socket_client, (void*)&tempmsg, sizeof(tempmsg), 0);
                                break;
                            }
                        }
                        break;
                    }
                    /* Tin nhắn thường */
                    for(int j = 0; j< MAX_CLIENTS; j++)
                    {
                        if(client[j].socket_client != fd)
                        {
                            send(client[j].socket_client, (void*)&tempmsg, sizeof(tempmsg), 0);
                        }
                    }
                }
            }
        }
    }
}
int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("Enter port number correct");
        exit(1);
    }
    int port = atoi(argv[1]);

    fd_set readfds;
    struct sockaddr_in address;
    pthread_t clientHT;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        client[i].socket_client = 0;
    }
    if((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("Failed to create Master Socket");
        exit(1);
    }
    address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
    addrlen = sizeof(address);
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	{
		printf("Faild to Bind Socket");
        exit(1);
	}

    printf("Listener on port %d \n", port);

    if(listen(master_socket, 10) < 0)
    {
        printf("Failed to listen on port %d\n", port);
        exit(1);
    }
    pthread_create(&clientHT, NULL, HandlerThread, NULL);
    pthread_detach(clientHT);
    printf("Start waiting for connections\n");

    while(1){
        /* Handler việc client kết nối tới */
        if((new_client = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
        {
            printf("Failed to accept client");
            exit(1);
        }
        printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_client , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(client[i].socket_client == 0){
                client[i].socket_client = new_client;
                break;
            }
        }
    }
}