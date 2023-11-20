/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

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
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

/* ====================== doit ====================== */

void doit(int fd){
  int is_static;     // 정적파일인지 아닌지를 판단해주기 위한 변수
  struct stat sbuf;  // 파일에 대한 정보를 가지고 있는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);            // rio(robust I/O (Rio)) 초기화
  Rio_readlineb(&rio, buf, MAXLINE);  // buf에서 client request 읽어들이기
  printf("Request headers:\n");
  printf("%s", buf);                  // request header 출력


  // buf에 있는 데이터를 method, uri, version에 담기
  sscanf(buf, "%s %s %s", method, uri, version);

  // method가 GET이 아니라면 error message 출력
  if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);  

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);  // static인지 dynamic인지 플래그 설정

  // filename에 맞는 정보 조회를 하지 못했으면 error message 출력
  if (stat(filename, &sbuf) < 0) {
      clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
      return;
  }

  // static contents
  if (is_static) {
    // file이 정규파일이 아니거나 사용자 읽기가 안되면 error message 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // response static file
    serve_static(fd, filename, sbuf.st_size, method);
  }
  // dynamic contents
  else {
    // file이 정규파일이 아니거나 사용자 읽기가 안되면 error message 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
  }
    // response dynamic files
    serve_dynamic(fd, filename, cgiargs, method);
  }
}

// error 발생 시, client에게 보내기 위한 response (error message)
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  char buf[MAXLINE], body[MAXBUF];

  // response body 쓰기 (HTML 형식)
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  //response header 쓰기
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);           // 버전, 에러번호, 상태메시지
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));  // 빈 줄을 생성해서 헤더 종료
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));                             // body 입력
}

// request header를 읽기 위한 함수
void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE];

  // 빈 텍스트 줄이 아닐 때까지 읽기
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;  // 아무것도 리턴하지 않는다. -> 무시
}

/* ====================== parse_uri ====================== */

// uri parsing을 하여 static을 request하면 0, dynamic을 request하면 1반환
int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;

  // static file request인 경우 
  if (!strstr(uri, "cgi-bin")) {  // uri에 cgi-bin(동적 파일 디렉토리)이 포함이 되어 있지 않으면
    strcpy(cgiargs, "");      // 인자가 없음을 표시
    strcpy(filename, ".");    // 상위 주소를 불러오고 . 추가
    strcat(filename, uri);    // filename에 uri 합치기

    // request에서 특정 static contents를 요구하지 않은 경우
    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "home.html");  //  기본 값 반환 (home page 반환)
    }
    return 1;
  }
  // dynamic file request인 경우
  else {
    ptr = index(uri, '?');      // uri부분에서 file name과 args를 구분하는 ?위치 찾기

    // ?가 있으면
    if (ptr) {
      strcpy(cgiargs, ptr+1);   //cgiargs에 인자 넣어주기
      *ptr = '\0';              // 포인터 ptr은 null처리
    }
    // ?가 없으면
    else {
      strcpy(cgiargs, "");
    }

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

/* ====================== serve_static ====================== */

// static content 처리
void serve_static(int fd, char *filename, int filesize, char *method){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);                           // 파일 타입 결정
  sprintf(buf, "HTTP/1.0 200 OK\r\n");                        // sprintf buf에 저장하겠다는 함수
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);         // buf 뒤에 계속해서 써서 buf에 저장하겠다
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);  // \r\n\r\n 여기만 두 줄을 만드는데 이게 빈 줄 하나를 만들어서 종료하겠다는 의미
  Rio_writen(fd, buf, strlen(buf));                           // request 헤더로 보낸다
  printf("Response headers:\n");
  printf("%s", buf);

  // HEAD 요청이면 body를 만들지 않고 종료
  if (strcasecmp(method, "HEAD") == 0) return;
    
  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);     // filename을 열고 srcfd를 얻어온다
  srcp = (char*)Malloc(filesize);          // malloc에 할당
  Rio_readn(srcfd, srcp, filesize);        // srcfd를 srcp에 복사
  Close(srcfd);                            // 가상메모리에 매핑했기 때문에 srcfd는 닫아준다
  Rio_writen(fd, srcp, filesize);          // 가상메모리에 매핑한 src파일을 fd에 저장
  free(srcp);                              // alloc한 메모리를 free해준다
}

// serve_static의 헬퍼 함수
void get_filetype(char *filename, char *filetype){
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpg"))
    strcpy(filetype, "video/mpg");
  else if (strstr(filename, ".mp4"))  // 다양한 파일 형식을 추가할 수 있다
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

/* ====================== serve_dynamic ====================== */

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method){
  char buf[MAXLINE], *emptylist[] = { NULL };

  // response 헤더는 부모가 보내주지만 나머지는 CGI에서 보내주어야 한다
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // 자식을 생성한다
  if (Fork() == 0) {
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);   // 인자를 setenv로 받고
    setenv("REQUEST_METHOD", method, 1);  // method 종류도 전달
    Dup2(fd, STDOUT_FILENO);              // 클라이언트에게 전달할 통로를 연결한다
    Execve(filename, emptylist, environ); // CGI 프로그램 실행!
  }

  // 여기는 부모
  Wait(NULL); /* Parent waits for and reaps child */
}