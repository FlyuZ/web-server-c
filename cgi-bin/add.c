/*
 * @filename:    add.c
 * @author:      Flyuz
 * @date:        2018年6月25日
 * @description: CGI-通用网关接口
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <ctype.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <sys/stat.h>
 #include <sys/mman.h>

#define	MAXLINE	 8192  /* Max text line length */

int main(int argc, char **argv) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }

  /* Make the response body */
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2,
          n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
