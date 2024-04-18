#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include "util.h"

const int MAX_EVENTS = 1024;
const int READ_BUFFER = 1024;

void set_non_blocking(int fd){
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

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
    
    // 创建epoll内核事件表
    int epfd = epoll_create1(0);
    epoll_event events[MAX_EVENTS], ev;

    // 注册服务器fd的可读事件
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (true) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            // 如果是服务器fd发生事件，说明有新客户端连接
            if (events[i].data.fd == sockfd) {
                sockaddr_in clnt_addr;
                socklen_t clnt_addr_len = sizeof(clnt_addr);
                bzero(&clnt_addr, sizeof(clnt_addr));

                // accept函数对于新的连接，创建一个新的socket并返回它
                int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                errif(clnt_sockfd == -1, "socket accept error");
                // inet_ntoa和ntohs是转换回主机字节序的函数
                printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
            
                bzero(&ev, sizeof ev);
                ev.data.fd = clnt_sockfd;
                // 对于客户端连接，使用ET模式
                ev.events = EPOLLIN | EPOLLET;
                // ET模式需要搭配非阻塞式IO
                set_non_blocking(clnt_sockfd);
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);
                
            } else if (events[i].events & EPOLLIN) { // 客户端有可读事件
                char buf[READ_BUFFER];
                while (true) {
                    memset(buf, 0, sizeof buf);               
                    // 从负责与客户端通信的socket读到buffer，返回已读数据大小
                    ssize_t read_bytes = read(events[i].data.fd, buf, sizeof(buf));
                    if(read_bytes > 0){
                        printf("message from client fd %d: %s\n", events[i].data.fd, buf);
                        write(events[i].data.fd, buf, sizeof(buf));
                    } else if(read_bytes == -1 && errno == EINTR){  //客户端正常中断、继续读取
                        printf("continue reading");
                        continue;
                    } else if(read_bytes == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
                        printf("finish reading once, errno: %d\n", errno);
                        break;
                    } else if(read_bytes == 0){  //EOF，客户端断开连接
                        printf("EOF, client fd %d disconnected\n", events[i].data.fd);
                        close(events[i].data.fd);   //关闭socket会自动将文件描述符从epoll树上移除
                        break;
                    }
                }
            } else {
                printf("something else happened");
            }
        }
    }
    close(sockfd);
    return 0;
}
