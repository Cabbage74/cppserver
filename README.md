# MyCppServer

Cpp入门项目，学习为主

## Unix下的五种IO模型

### 阻塞式I/O

![最常见的](https://s2.loli.net/2024/04/18/BFNoJLhwDpc5GA2.png)

### 非阻塞式I/O

![轮询内核，耗费大量CPU时间](https://s2.loli.net/2024/04/18/yQOz7A64CE1hb3R.png)

### I/O复用

![阻塞于select直到套接字变为可读，然后调用recvfrom](https://s2.loli.net/2024/04/18/3XBrucNHQMsAvbV.png)

与I/O复用密切相关的另一种I/O模型是在多线程中使用阻塞式I/O，每个文件描述符一个线程，每个线程里自由地调用阻塞式I/O系统调用

### 信号驱动式I/O

![优势在于等待数据报达到期间进程不被阻塞](https://s2.loli.net/2024/04/18/zCkNbeduSTVXvD6.png)

### 异步I/O

![告诉内核启动某个操作，并让内核在整个操作完成后通知我们](https://s2.loli.net/2024/04/18/OyQnkuIMJP9gF6L.png)

与信号驱动式I/O的区别是前者由内核通知我们何时可以启动一个I/O操作，而异步I/O是由内核通知我们I/O操作何时完成

### 比较

![](https://s2.loli.net/2024/04/18/rFos7xk8TMaIWVE.png)

## IO多路复用

I/O复用：内核一旦发现进程指定的一个或多个I/O条件就绪，就通知进程

### select

该函数允许进程指示内核等待多个事件中的任何一个发生，并只在有一个或多个事件发生或经历一段指定的时间后才唤醒它

作为一个例子，我们可以调用select，告知内核在下列情况的某一列发生时就返回：

- 集合{1, 4, 5}中的任何描述符准备好读

- 集合{2, 7}中的任何描述符准备好写

- 集合{1, 4}中的任何描述符有异常条件待处理

- 已经历10.2秒

select原型

``` cpp
#include <sys/select.h>
#include <sys/time.h>
// select的返回值是所有bitmap的已就绪的总位数，如果超时就返回零，出错就返回-1
int select(int maxfdpl, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout);
```

select可用于不限于套接字的描述符

fd_set其实就是一个bitmap，以值域为轴，需要关心的位就置为1

FD_SETSIZE这个宏常值被设为了1024，所以不修改宏的话select最多关心这么多的描述符，修改完宏还要重新编译内核

select的bitmap大致是这样工作的：

假如我把readset的某些位置为1

select返回后，会修改readset以反映哪些文件描述符是ready的

如果某个文件描述符可以读了，那么这一位仍然是1，否则就被置为0

这就是为什么每次重新调用select需要把bitmap重新初始化

### poll

poll原型

``` cpp
#include <poll.h>

struct pollfd {
    int fd; // 要检查的描述符
    short events; // 关心的事件
    short revents; // 实际发生的事件
}

// poll的返回值是就绪描述符的数目，如果超时就返回零，出错就返回-1
int poll(struct pollfd *fdarray, unsigned long nfds, int timeout);
```

poll相比select少了最大描述符数的限制，还避免了对原始文件描述符集的修改(即fd和event字段)，每次调用poll还不用重新清空revents字段

### epoll

epoll是linux特有的

epoll使用一组函数来完成任务，而不是单个函数

epoll把用户关心的文件描述符上的事件放在内核里的一个事件表中，从而无须像select和poll那样每次调用都要重复传入关心的文件描述符集

但epoll需要使用一个额外的文件描述符，来唯一标识内核中的这个事件表，由epoll_create函数创建

``` cpp
#include <sys/epoll.h>
// size是一个被历史淘汰的参数
int epoll_create(int size)
```

epoll_ctl操作epoll的内核事件表

``` cpp
#include <sys/epoll.h>
// 成功时返回0，失败时返回-1
int epoll_ctl(int epfd,int op,int fd,struct epoll_event*event)
```

op参数指定操作类型，fd是要操作的文件描述符

EPOLL_CTL_ADD往事件表中注册fd上的事件

EPOLL_CTL_MOD修改fd上的注册事件

EPOLL_CTL_DEL删除fd上的注册事件

event参数指定事件

``` cpp
struct epoll_event {
    __uint32_t events; // epoll事件
    epoll_data_t data; // 用户数据
}

typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
```

events成员描述事件类型，如EPOLLIN为可读事件

epoll_data_t是一个联合体，用的最多的成员是fd，用来指定事件所从属的目标文件描述符

ptr成员可用来指定与fd相关的用户数据，因为联合体只能维护一个成员，所以要用ptr的话得把fd放在ptr指向的用户数据里

epoll_wait函数在一段超时时间内等待一组文件描述符上的事件

``` cpp
#include <sys/epoll.h>
// 成功时返回就绪描述符个数，失败时返回-1
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

epoll_wait如果检测到事件，就将所有就绪的事件从内核事件表中复制到第二个参数events指向的数组里，大大提高了索引就绪文件描述符的效率

``` cpp
// poll vs. epoll

// poll
int ret=poll(fds,MAX_EVENT_NUMBER,-1);
for(int i=0;i＜MAX_EVENT_NUMBER;++i) {
    /*判断第i个文件描述符是否就绪*/
    if(fds[i].revents＆POLLIN) {
        int sockfd=fds[i].fd;
        /*处理sockfd*/
    }
}

// epoll
int ret=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
/*仅遍历就绪的ret个文件描述符*/
for(int i=0;i＜ret;i++) {
    int sockfd=events[i].data.fd;
    /*sockfd肯定就绪，直接处理*/
}
```

epoll不仅支持Level Trigger(LT)模式，还支持Edge Trigger(ET)模式

LT模式是默认模式，这种模式下可看作高效版的poll

当往epoll内核事件表中注册一个文件描述符上的EPOLLET事件时，epoll将以ET模式来操作该文件描述符

ET模式是epoll的高效工作模式

对于采用LT工作模式的文件描述符，当epoll_wait检测到其上有事件发生并将此事件通知应用程序后，应用程序可以不立即处理该事件

这样，当应用程序下一次调用epoll_wait时，epoll_wait还会再次向应用程序通告此事件，直到该事件被处理

而对于采用ET工作模式的文件描述符，当epoll_wait检测到其上有事件发生并将此事件通知应用程序后，应用程序必须立即处理该事件，因为后续的epoll_wait调用将不再向应用程序通知这一事件

注意：每个使用ET模式的文件描述符都应该是非阻塞的，如果文件描述符是阻塞的，那么读或写操作将会因为没有后续的事件而一直处于阻塞状态（饥渴状态）

``` cpp
// 水平触发和边缘触发可以混用
struct epoll_event event;
int fd1 = ...; // 文件描述符1
int fd2 = ...; // 文件描述符2

// 对fd1使用水平触发
event.events = EPOLLIN; // 默认为水平触发
event.data.fd = fd1;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd1, &event);

// 对fd2使用边缘触发
event.events = EPOLLIN | EPOLLET; // 使用 EPOLLET 标志设置边缘触发
event.data.fd = fd2;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd2, &event);
```

即使我们使用ET模式，一个socket上的某个事件还是可能被触发多次

这在并发程序中就会引起一个问题，比如一个线程（或进程，下同）在读取完某个socket上的数据后开始处理这些数据，而在数据的处理过程中该socket上又有新数据可读（EPOLLIN再次被触发），此时另外一个线程被唤醒来读取这些新的数据，于是就出现了两个线程同时操作一个socket的局面，这是不被希望的

对于注册了EPOLLONESHOT事件的文件描述符，操作系统最多触发其上注册的一个可读、可写或者异常事件，且只触发一次，除非我们使用epoll_ctl函数重置该文件描述符上注册的EPOLLONESHOT事
件

这样，当一个线程在处理某个socket时，其他线程是不可能有机会操作该socket的，但反过来思考，注册了EPOLLONESHOT事件的socket一旦被某个线程处理完毕，该线程就应该立即重置这个socket上的EPOLLONESHOT事件，以确保这个socket下一次可读时，其EPOLLIN事件能被触发，进而让其他工作线程有机会继续处理这个socket

有点像“锁”

### 比较

![](https://s2.loli.net/2024/04/18/vxSoBzCMpVgk1Fh.png)
