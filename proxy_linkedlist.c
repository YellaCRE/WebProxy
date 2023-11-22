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
void do_request(int p_clientfd, char *path, char *host);
void do_response(int p_connfd, int p_clientfd, char *path, char *host, char *port);
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

  Rio_readinitb(&rio, p_connfd);           
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
    delete(node);                                     // 노드 삭제하고
    enqueue(node);                                    // 가장 앞에 삽입
    Rio_writen(p_connfd, node->payload, node->size);  // client에게 cache 내용을 보낸다
    return;
  }

  // Cache miss이면
  printf("Cache miss!\n");
  p_clientfd = open_clientfd(host, port);
  do_request(p_clientfd, path, host);
  do_response(p_connfd, p_clientfd, path, host, port);

  Close(p_clientfd);
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

void do_response(int p_connfd, int p_clientfd, char *path, char *host, char *port){
  size_t n;
  char buf[MAX_CACHE_SIZE];
  rio_t rio;

  cnode_t * node;

  Rio_readinitb(&rio, p_clientfd);
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);
  Rio_writen(p_connfd, buf, n);

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