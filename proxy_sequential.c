#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* ====================== predefine ====================== */

void do_it(int fd);
void do_request(int proxy_clientfd, char *path, char *host);
void do_response(int proxy_connfd, int proxy_clientfd);
int parse_uri(char *uri, char *path, char *host, char *port);

/* ====================== main ====================== */

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  // 반복실행 서버, 연결 요청을 계속 받는다
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    do_it(connfd);
    Close(connfd);
  }
  return 0;
}

/* ====================== doit ====================== */

void do_it(int proxy_connfd){
  int proxy_clientfd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char path[MAXLINE], host[MAXLINE], port[MAXLINE];
  rio_t rio;
  
  /* Read request line and headers from Client */
  Rio_readinitb(&rio, proxy_connfd);           
  Rio_readlineb(&rio, buf, MAXLINE);       
  printf("Request headers:\n");
  printf("%s", buf);

  // buf에 있는 데이터를 method, uri, version에 담기
  sscanf(buf, "%s %s %s", method, uri, version); 

  /* Parse URI from GET request */
  parse_uri(uri, path, host, port);

  proxy_clientfd = open_clientfd(host, port);             // proxy_clientfd = proxy의 proxy_clientfd (연결됨)
  do_request(proxy_clientfd, path, host);             // proxy_clientfd에 Request headers 저장과 동시에 server의 connfd에 쓰여짐
  do_response(proxy_connfd, proxy_clientfd);                  
  Close(proxy_clientfd);                                  // proxy_clientfd 역할 끝
}

/* ====================== parse_uri ====================== */

int parse_uri(char *uri, char *path, char *host, char *port){ 
  char *ptr;

  if (!(ptr = strstr(uri, "://"))) // http:// 가 있는지 확인
    return -1;
  ptr += 3;                       
  strcpy(host, ptr);               // host = www.google.com:80/index.html

  if((ptr = strchr(host, ':'))){   // strchr(): 문자 하나만 찾는 함수 (''작은따옴표사용)
    *ptr = '\0';                   // host = www.google.com
    ptr += 1;
    strcpy(port, ptr);             // port = 80/index.html
  }
  else{
    if((ptr = strchr(host, '/'))){
      *ptr = '\0';
      ptr += 1;
    }
    strcpy(port, "80");           // port 디폴트 값
  }

  if ((ptr = strchr(port, '/'))){ // port = 80/index.html
    *ptr = '\0';                  // port = 80
    ptr += 1;     
    strcpy(path, "/");        // path = /
    strcat(path, ptr);        // path = /index.html
  }  
  else strcpy(path, "/");

  return 0; // function int return => for valid check
}

/* ====================== do_request ====================== */

void do_request(int proxy_clientfd, char *path, char *host){
  char *version = "HTTP/1.0";
	char buf[MAXLINE];

  /*Set header*/
	sprintf(buf, "GET %s %s\r\n", path, version);
	sprintf(buf, "%sHost: %s\r\n", buf, host);    
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnections: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);

  Rio_writen(proxy_clientfd, buf, strlen(buf));
}

/* ====================== do_response ====================== */

void do_response(int proxy_connfd, int proxy_clientfd){
  size_t n;                  // size_t ssize_t 똑같다
  char buf[MAX_CACHE_SIZE];  // MAXLINE을 하면 8점이 나온다
  rio_t rio;

  Rio_readinitb(&rio, proxy_clientfd);
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);  // Rio_readlineb이 아니라 Rio_readnb
  Rio_writen(proxy_connfd, buf, n);
}