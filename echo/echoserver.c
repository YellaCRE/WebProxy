#include "csapp.h"

// 에코 함수 정의
void echo(int connfd){  
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    rio_readinitb(&rio, connfd);  // connfd와 연결하고 rio(robust I/O (Rio)) 초기화

    // EOF를 만날 때까지 반복
    while((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {  // rio의 내용을 user buffer에 읽음
        printf("server received %d bytes\n", (int)n);
        rio_writen(connfd, buf, n);                        // user buffer의 내용을 connfd에 적어서 네트워크로 보냄
    }
}
// connfd에서 읽은 것을 버퍼에 저장했다가 그대로 돌려주는 함수

int main(int argc, char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough space for any address */
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);  // listen을 호출하고 connect가 올 때까지 대기

    // connect가 오면 시작
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);  // accept로 connfd를 열고 연결한다

        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd);   // 에코 함수 실행

        Close(connfd);  // 에코 함수가 종료되면 connfd를 닫는다
    }
    exit(0);
}