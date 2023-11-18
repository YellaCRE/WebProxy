/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  // getenv로 인자를 버퍼로 받아 옴
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');          // &의 주소를 찾는다
    *p = '\0';                     // &를 '\0'으로 바꾼다
    sscanf(buf, "first=%d", &n1);  // buffer에서 %d를 n1으로 가져온다
    sscanf(p+1, "second=%d", &n2); // p+1 주소에 있는 값에서 %d를 n2로 가져온다
  }
  
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  
  printf("%s", content);  // 결과 출력
  fflush(stdout);         // 버퍼 비우기
  exit(0);
}
/* $end adder */
