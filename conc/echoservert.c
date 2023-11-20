#include "csapp.h"

void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv) {
    int listenfd, *connfdp, port;
    socklen_t clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    while (1) {
        connfdp = Malloc(sizeof(int));                                  // line:conc:echoservert:beginmalloc
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);    // line:conc:echoservert:endmalloc
        Pthread_create(&tid, NULL, thread, connfdp);                    // 쓰레드 생성하는데 thread(connfdp)로 생성된다
    }
}

/* thread routine */
void *thread(void *vargp) {  
    int connfd = *((int *)vargp);   // 

    Pthread_detach(pthread_self()); // detach를 통해 자원 반환
    Free(vargp);                    // connfdp를 free
    echo(connfd);
    Close(connfd);

    return NULL;
}