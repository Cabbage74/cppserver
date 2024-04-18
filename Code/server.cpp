#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "util.h"

int main() {
    // 创建socket，参数依次为IP地址类型，数据传输方式，协议(0表示由前两个参数推导)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // inet_addr和htons是把主机字节序转换成网络字节序(大端)的函数
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);

    // 把socket地址和文件描述符绑定
    // sockaddr_in是IPV4专用地址
    // socket接口需要支持多种不同的协议，使用sockaddr作为一个通用接口可以让同一个函数支持多种不同的地址类型
    // 所以要强制类型转换
    errif(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");

    // 监听socket，第二个参数是listen函数的最大监听队列长度
    errif(listen(sockfd, SOMAXCONN) == -1, "socket listen error");
    
    sockaddr_in clnt_addr;
    socklen_t clnt_addr_len = sizeof(clnt_addr);
    bzero(&clnt_addr, sizeof(clnt_addr));

    // accept函数对于新的连接，创建一个新的socket并返回它
    int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
    errif(clnt_sockfd == -1, "socket accept error");

    // inet_ntoa和ntohs是转换回主机字节序的函数
    printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
    
    while (true) {
        char buf[1024];
        memset(buf, 0, sizeof buf);               
        // 从负责与客户端通信的socket读到buffer，返回已读数据大小
        ssize_t read_bytes = read(clnt_sockfd, buf, sizeof(buf));
        if (read_bytes > 0) { //读到数据，打印出来，将数据写回客户端
            printf("message from client fd %d: %s\n", clnt_sockfd, buf);
            write(clnt_sockfd, buf, sizeof buf);
        } else if (read_bytes == 0) { // 表示EOF
            printf("client fd %d disconnected\n", clnt_sockfd);
            close(clnt_sockfd);
            break;
        } else if (read_bytes == -1) { // 表示read发生错误
            close(clnt_sockfd);
            errif(true, "socket read error");
        }
    }

    return 0;
}
