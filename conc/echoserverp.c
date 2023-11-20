#include "csapp.h"

void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    rio_readinitb(&rio, connfd);

    while((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        rio_writen(connfd, buf, n);
    }
}

void sigchild_handler(int sig){
    while (waitpid(-1, 0, WNOHANG) > 0)
        ;
    return;
}

int main(int argc, char **argv) {
    int listenfd, connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);               // port 번호를 받으면
    signal(SIGCHLD, sigchild_handler);  // sigchild 핸들러 돌리고
    listenfd = Open_listenfd(port);     // listenfd를 연다
    
    while (1) {
        connfd = Accept(listenfd, (SA *) &clientaddr, clientlen);  // accept를 받으면 자식 생성
        
        if (Fork() == 0) {
            Close(listenfd);  // close the listenfd of child
            echo(connfd);     // service to client of child
            Close(connfd);    // close the connfd of child
            exit(0);          // child exits
        }

        Close(connfd);  //(important) close the connfd of parent
    }
}