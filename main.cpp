#include <stdio.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "network/NetManager.h"

#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("error no: %d error msg %s\n", errno, strerror(errno)); exit(1);}

const int kReadEvent = 1;
const int kWriteEvent = 2;

void setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    exit_if(flags < 0, "fcntl failed");

    int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    exit_if(r < 0, "fcntl failed");
}

void updateEvents(int efd, int fd, int events, bool modify) {
    struct kevent ev[2];
    int n = 0;
    
    if(events & kReadEvent) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)fd);
    } else if(modify) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)fd);
    }

    if(events & kWriteEvent) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)fd);
    } else if(modify) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void *)(intptr_t)fd);
    }

    printf("%s fd %d events read %d write %d\n",
            modify ? "mod" : "add", fd, events & kReadEvent, events & kWriteEvent);
    int r = kevent(efd, ev, n, NULL, 0, NULL);
    exit_if(r, "kevent failed");
}

void handleAccept(int efd, int fd) {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int cfd = accept(fd, (struct sockaddr *)&raddr, &rsz);
    exit_if(cfd < 0, "accept failed");

    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int r = getpeername(cfd, (sockaddr *)&peer,  &alen);
    exit_if(r < 0, "getpeername failed");
    printf("accept a connection from %s\n", inet_ntoa(raddr.sin_addr));

    setNonBlock(cfd);
    updateEvents(efd, cfd, kReadEvent, false);
}

void handleRead(int efd, int fd) {
    char buf[4096];
    int n = 0; 

    while( (n = ::read(fd, buf, sizeof buf)) > 0 ) {
        printf("fd %d read %d bytes\n", fd, n);

        ::write(fd, buf, n);
    }

    if( n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) ) {
        return;
    }
    exit_if(n < 0, "read error");
    printf("fd %d closed\n", fd);
    close(fd);
}

void handleWrite(int efd, int fd) {
//    printf("WRITE EVENT ======>\n");
//    updateEvents(efd, fd, kReadEvent, true);
}

void loop_once(int efd, int lfd, int waitms) {
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;
    const int kMaxEvents = 20;
    struct kevent activeEvs[kMaxEvents];
    int n = kevent(efd, NULL, 0, activeEvs, kMaxEvents, &timeout);
//    printf("epoll_wait return %d\n", n);

    for(int i = 0; i < n; i++) {
        int fd = (int)(intptr_t)activeEvs[i].udata;
        int events = activeEvs[i].filter;

        if(events == EVFILT_READ) {
            if(fd == lfd) {
                handleAccept(efd, fd);
            } else {
                handleRead(efd, fd);
            }
        } else if(events == EVFILT_WRITE) {
            handleWrite(efd, fd);
        } else {
            exit_if(1, "unknown event");
        }
    }
}

int main() {
    printf("Test VPS\n");

    NetManager *pNetMgr = NULL;

    pNetMgr = new NetManager();
    pNetMgr->OnServerModeStart();

    // short port = 10001;
    // int epollfd = kqueue();
    // exit_if(epollfd < 0, "epoll_create failed");

    // int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // exit_if(listenfd < 0, "socket failed");

    // struct sockaddr_in addr;
    // memset(&addr, 0, sizeof addr);
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = INADDR_ANY;

    // int r = ::bind(listenfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    // exit_if(r, "bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));

    // r = listen(listenfd, 20);
    // exit_if(r, "listen failed %d %s", errno, strerror(errno));

    // printf("fd %d listening at %d\n", listenfd, port);

    // setNonBlock(listenfd);
    // updateEvents(epollfd, listenfd, kReadEvent, false);

    if(pNetMgr != NULL) {
        delete pNetMgr;
        pNetMgr = NULL;
    }

    // for(;;) {
    //     loop_once(epollfd, listenfd, 10000);
    // }

    return 0;
}