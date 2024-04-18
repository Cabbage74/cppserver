#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "util.h"


int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);

    errif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");    
    
    while (true) {
        char buf[1024];
        memset(buf, 0, sizeof buf);
        scanf("%s", buf);
        // 将buf中的数据写入与服务器通信的socket，返回已发送数据大小
        ssize_t write_bytes = write(sockfd, buf, sizeof buf);
        if (write_bytes == -1) {
            printf("socket already disconnected, can't write any more!\n");
            break;
        }
        memset(buf, 0, sizeof buf);
        // 从与服务端通信的socket读到buf，返回已读数据大小
        ssize_t read_bytes = read(sockfd, buf, sizeof buf);
        if (read_bytes > 0) {
            printf("message from server: %s\n", buf);
        } else if (read_bytes == 0) {
            printf("server socket disconnected!\n");
            break;
        } else if (read_bytes == -1) {
            close(sockfd);
            errif(true, "socket read error");
        }
    }

    return 0;
}
