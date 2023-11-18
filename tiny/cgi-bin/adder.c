/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p, *method;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  // getenv로 인자를 버퍼로 받아 옴
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);  // 버퍼의 인자를 가져온다
    strcpy(arg2, p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }
  
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);  // \r\n 커서를 앞으로 보내고 엔터를 친다
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");

  // method가 GET일 경우에만 response body 보냄
  if(strcasecmp(method, "GET") == 0) { 
    printf("%s", content);
  }
  fflush(stdout);

  exit(0);
}
/* $end adder */
