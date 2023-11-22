#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* ====================== predefine ====================== */

void *thread(void *vargp);
void do_it(int fd);
void do_request(int p_clientfd, char *path, char *host);
void do_response(int p_connfd, int p_clientfd);
int parse_uri(char *uri, char *path, char *host, char *port);

/* ====================== main ====================== */

int main(int argc, char **argv) {
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  pthread_t tid;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // cache 생성
  cache_init();

  listenfd = Open_listenfd(argv[1]);

  // 반복실행 서버, 연결 요청을 계속 받는다
  while (1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));     
    *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); 

    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    Pthread_create(&tid, NULL, thread, connfdp);         // 다중 쓰레딩을 통한 conncurrent 접속 구현
  }
  return 0;
}

void* thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);    // free를 꼭 해주어야 한다
    do_it(connfd);
    Close(connfd);
    return NULL;    // NULL 리턴도 중요하다
  }

/* ====================== doit ====================== */

void do_it(int p_connfd){
  int p_clientfd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char path[MAXLINE], host[MAXLINE], port[MAXLINE];
  rio_t rio;

  cnode_t * node;
  char payload[MAX_OBJECT_SIZE];
  size_t n;                  // size_t ssize_t 똑같다
  char res_buf[MAX_CACHE_SIZE];  // MAXLINE을 하면 8점이 나온다
  rio_t res_rio;

  /* Read request line and headers from Client */
  Rio_readinitb(&rio, p_connfd);           
  Rio_readlineb(&rio, buf, MAXLINE);       
  printf("Request headers:\n");
  printf("%s", buf);

  // buf에 있는 데이터를 method, uri, version에 담기
  sscanf(buf, "%s %s %s", method, uri, version); 

  /* Parse URI from GET request */
  parse_uri(uri, path, host, port);

  /* ====================== caching ====================== */

  Cache_check();
  if ((node = match(host, port, path)) != NULL) {
    printf("Cache hit!\n");
    delete(node);
    enqueue(node);
    Rio_writen(p_connfd, node->payload, node->size);
    printf("Senting respond %u bytes from cache\n",
            (unsigned int)node->size);

    printf("Proxy is exiting\n\n");
    return;
  }

  // 캐시에 없으면
  printf("Cache miss!\n");

  p_clientfd = open_clientfd(host, port);             // p_clientfd = proxy의 clientfd (연결됨)
  do_request(p_clientfd, path, host);             // p_clientfd에 Request headers 저장과 동시에 server의 connfd에 쓰여짐

  // do_response
  Rio_readinitb(&res_rio, p_clientfd);
  n = Rio_readnb(&res_rio, res_buf, MAX_CACHE_SIZE);  // Rio_readlineb이 아니라 Rio_readnb
  Rio_writen(p_connfd, res_buf, n);

  strcpy(payload, "");
  if (n <= MAX_OBJECT_SIZE) {
    strcat(payload, res_buf);
    node = new(host, port, path, payload, n);

    Cache_check();            
    while (cache_load + n > MAX_CACHE_SIZE) {
        printf("!!!!!!!!!!!!!!!!!Cache evicted!!!!!!!!!!!!!!!!!!\n");
        dequeue();
    }
    enqueue(node);
    Cache_check();
  }
  /* ====================== doit ====================== */

  Close(p_clientfd);
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

void do_request(int p_clientfd, char *path, char *host){
  char *version = "HTTP/1.0";
	char buf[MAXLINE];

  /*Set header*/
	sprintf(buf, "GET %s %s\r\n", path, version);
	sprintf(buf, "%sHost: %s\r\n", buf, host);    
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnections: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);

  Rio_writen(p_clientfd, buf, strlen(buf));
}

/* ====================== do_response ====================== */

void do_response(int p_connfd, int p_clientfd){
  size_t n;                  // size_t ssize_t 똑같다
  char buf[MAX_CACHE_SIZE];  // MAXLINE을 하면 8점이 나온다
  rio_t rio;

  Rio_readinitb(&rio, p_clientfd);
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);  // Rio_readlineb이 아니라 Rio_readnb
  Rio_writen(p_connfd, buf, n);

  return n;
}