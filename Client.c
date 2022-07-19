#include<stdio.h>
#include<sys/socket.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#define MAX 1024
#define MAX_NAME 10
int running = 1;
int listen_fd;
char name[MAX_NAME];
char *prvCMD = "PRV*";
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
void catch_CTRL_C(int sig){
    running = 0;
}
/* Thay kí tự \n bằng \0 */
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}
/* Thread handler xử lý gửi message */
void *send_msg_handler(void *args){
    char buffer[MAX] = {0};
    MSG sMSG = {0};
    char *pname;
    char *msg;
    strcpy(sMSG.name, name);
    while(1){
        fflush(stdout);
        fflush(stdin);
        fgets(buffer, MAX, stdin);
        str_trim_lf(buffer, MAX);
        if(strstr(buffer, prvCMD) != NULL){
            /* Private Message */
            pname = strtok(buffer, "*");
            pname = strtok(NULL,"*");
            msg = strtok(NULL,"*");
            if(pname == NULL || msg == NULL){
                printf("Syntax error\n");
                continue;
            }
            sMSG.ACTION = PRIMSG;
            strcpy(sMSG.name, name);
            strcpy(sMSG.pname, pname);
            strcpy(sMSG.msg, msg);
            send(listen_fd, (void*)&sMSG, sizeof(sMSG), 0);
            memset(&sMSG, 0, sizeof(sMSG));  
            continue;
        }
        strcpy(sMSG.msg, buffer);
        send(listen_fd, (void*)&sMSG, sizeof(sMSG), 0);
        memset(&sMSG, 0, sizeof(sMSG));  
    }
}
void *recv_msg_handler(void *args){
    MSG sMSG = {0};
    while(1){
        int receive = recv(listen_fd, (void*)&sMSG, sizeof(sMSG), 0);
        if(receive <= 0){
            exit(1);
        }else{
            if(sMSG.ACTION == PRIMSG){
                fflush(stdout);
                printf("(PRIVATE) %s: %s\n", sMSG.name, sMSG.msg);
                fflush(stdout);
                memset(&sMSG, 0, sizeof(sMSG));
                continue;
            }
            printf("%s: %s\n", sMSG.name, sMSG.msg);
            fflush(stdout);
        }
        memset(&sMSG, 0, sizeof(sMSG));
    }
}
/* Thread handler xử lý nhận message */
int main(int argc, char *argv[]){
    MSG nMSG = {0};
    if(argc != 2){
        printf("Enter port number correct");
        exit(1);
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    signal(SIGINT, catch_CTRL_C);

    struct sockaddr_in serverAddress;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(ip);
	serverAddress.sin_port = htons(port);
    printf("Nhập tên: ");
    fgets(name, MAX_NAME, stdin);
    str_trim_lf(name, MAX_NAME);
    if (connect(listen_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        printf("Connection with the server failed...\n");
        exit(0);
    } else{
        printf("Connected to Server\n");
    }
    strcpy(nMSG.name, name);
    nMSG.ACTION = INIT;
    send(listen_fd, (void*)&nMSG, sizeof(nMSG), 0);
    printf("====Welcome to chat room =====\n");
    printf("Syntax: PRV*NAME*MESSAGE to send private message\n");
    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, send_msg_handler, NULL) != 0){
        printf("ERROR create send msg\n");
        exit(1);
    }
    pthread_detach(send_msg_thread);
    pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, recv_msg_handler, NULL) != 0){
        printf("ERROR create send msg\n");
        exit(1);
    }
    pthread_detach(recv_msg_thread);
    while (1){
		if(running == 0){
			printf("\nBye\n");
			break;
        }
	}
	close(listen_fd);

}