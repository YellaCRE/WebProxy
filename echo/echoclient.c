#include "csapp.h"

int main(int argc, char **argv){
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;  // Robust I/O

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];  // host는 첫 번째 인자
    port = argv[2];  // port는 두 번째 인자

    clientfd = open_clientfd(host, port);  // connect 보내고 대기

    //connect가 종료되면 실행
    Rio_readinitb(&rio, clientfd);         // clientfd의 정보를 읽어서 init을 rio에 저장

    // fgets가 EOF 표준 입력을 만나면 종료 -> 다 읽었거나, 컨트롤 D 강제중지 당했을 때
    while (fgets(buf, MAXLINE, stdin) != NULL) {  // 사용자가 입력한 stdin을 버퍼에 저장
        Rio_writen(clientfd, buf, strlen(buf));   // 버퍼의 내용을 clientfd에 적어서 네트워크로 보냄
                                                  // 서버에서 rio로 connfd가 도착하면
        Rio_readlineb(&rio, buf, MAXLINE);        // rio의 내용을 버퍼에 옮긴다
        fputs(buf, stdout);                       // 버퍼의 내용을 stdout에 저장
    }
    // 루프가 종료되면 서버로 EOF를 보낸다, 서버는 rio_readlineb가 리턴 코드 0을 받음으로써 인지한다. 
    // 그리고 클라이언트 종료되고 클라이언트 커널이 자동으로 clientfd를 닫는다.

    Close(clientfd);  // 사실 불필요하지만 명시적으로 닫아주자
    exit(0);
}