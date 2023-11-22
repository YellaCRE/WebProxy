#include <stdio.h>
#include "csapp.h"
#include "cache.h"  // cache 활용

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
void do_request(int proxy_clientfd, char *path, char *host);
void do_response(int proxy_connfd, int proxy_clientfd, char *path, char *host, char *port);
int parse_uri(char *uri, char *path, char *host, char *port);

/* ====================== main ====================== */

int main(int argc, char **argv) {
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // cache 생성
  cache_init();

  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));                       // race 방지를 위해 malloc에 connfd를 저장해야한다
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

/* ====================== do_it ====================== */

void do_it(int proxy_connfd){
  int proxy_clientfd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char path[MAXLINE], host[MAXLINE], port[MAXLINE];
  rio_t rio;

  cnode_t * node;

  Rio_readinitb(&rio, proxy_connfd);           
  Rio_readlineb(&rio, buf, MAXLINE);       
  printf("Request headers:\n");
  printf("%s", buf);

  sscanf(buf, "%s %s %s", method, uri, version); 

  parse_uri(uri, path, host, port);

  /* ====================== caching ====================== */

  Cache_check();

  // Cache hit이면
  if ((node = match(host, port, path)) != NULL) {
    printf("Cache hit!\n");
    delete(node);                                         // 노드 삭제하고
    enqueue(node);                                        // 가장 앞에 삽입
    Rio_writen(proxy_connfd, node->payload, node->size);  // client에게 cache 내용을 보낸다
    return;
  }

  // Cache miss이면
  printf("Cache miss!\n");
  proxy_clientfd = open_clientfd(host, port);                   // proxy의 clientfd와 server의 connfd를 연결
  do_request(proxy_clientfd, path, host);                       // proxy의 clientfd에 Request 쓰임과 동시에 server의 connfd에 쓰여짐
  do_response(proxy_connfd, proxy_clientfd, path, host, port);  // host에게 Response를 보내고 캐시에 저장

  Close(proxy_clientfd);                                        // proxy_clientfd 명시적 종료
}

/* ====================== parse_uri ====================== */

int parse_uri(char *uri, char *path, char *host, char *port){ 
  char *ptr;

  if (!(ptr = strstr(uri, "://")))
    return -1;
  ptr += 3;                       
  strcpy(host, ptr);

  if((ptr = strchr(host, ':'))){
    *ptr = '\0';
    ptr += 1;
    strcpy(port, ptr);
  }
  else{
    if((ptr = strchr(host, '/'))){
      *ptr = '\0';
      ptr += 1;
    }
    strcpy(port, "80");
  }

  if ((ptr = strchr(port, '/'))){
    *ptr = '\0';
    ptr += 1;     
    strcpy(path, "/");
    strcat(path, ptr);
  }  
  else strcpy(path, "/");

  return 0;
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

  Rio_writen(proxy_clientfd, buf, strlen(buf));   // proxy_clientfd에 request header를 쓴다
}

/* ====================== do_response ====================== */

void do_response(int proxy_connfd, int proxy_clientfd, char *path, char *host, char *port){
  size_t n;
  char buf[MAX_CACHE_SIZE];
  rio_t rio;

  cnode_t * node;

  Rio_readinitb(&rio, proxy_clientfd);          
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);    // &rio에는 server에서 받아 온 내용이 있음
  Rio_writen(proxy_connfd, buf, n);             // proxy connfd에 &rio에 있는 내용을 쓴다

  // cache에 저장
  if (n <= MAX_OBJECT_SIZE) {
    node = new(host, port, path, buf, n);       // 버퍼의 내용으로 새로운 노드 생성

    Cache_check();            
    while (cache_load + n > MAX_CACHE_SIZE) {  // 자리가 빌 때까지 비우기
        dequeue();
    }
    enqueue(node);                             // 노드 저장
    Cache_check();
  }

  return n;
}