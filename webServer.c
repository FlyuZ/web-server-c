/*
 * @filename:    webServer.c
 * @author:      Flyuz
 * @date:        2018年6月25日
 * @description: 主程序
 */

#include "functionLib.h"

void doit(int fd);
void serve_static(int fd, char *filename, int filesize);
int parse_uri(char *uri, char *filename, char *cgiargs);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);


int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;

  struct sockaddr_in clientaddr;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  printf("The web server has been started.....\n");

  listenfd = Open_listenfd(argv[1]);
  /*
  信号处理函数
  用来处理僵尸进程
  */
  signal_r(SIGCHLD, sigchild_handler);

  while (1) {
    clientlen = sizeof(clientaddr);
    if((connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0)
    {
      if(errno == EINTR)
          continue;
      else
          printf("Accept error...");
    }
    pid_t pid = Fork();
    if(pid == 0)
    {
        doit(connfd);
        Close(connfd);
        exit(0);
    }
    else
    {
      Close(connfd);
    }
  }
}

void doit(int fd) {
  int is_static;
  struct stat sbuf;
  rio_t rio;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE]; //设置根目录
  char cgiargs[MAXLINE];

  //初始化 rio 结构
  Rio_readinitb(&rio, fd);
  //读取http请求行
  Rio_readlineb(&rio, buf, MAXLINE);
  //格式化存入 把该行拆分
  sscanf(buf, "%s %s %s", method, uri, version);

  //只能处理GET请求，如果不是GET请求的话返回错误
  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not Implemented",
                "Flyuz does not implement thid method");
    return;
  }

  //读取并忽略请求报头
  read_requesthdrs(&rio);

  // memset(filename,0,sizeof(filename));
  //解析 URI
  is_static = parse_uri(uri, filename, cgiargs);

  //文件不存在
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found",
                "Flyuz couldn't find this file");
    return;
  }

  if (is_static) { //服务静态内容
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Flyuz couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  } else { //服务动态内容
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Flyuz couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

/*
 * 读取http 请求报头，无法使用请求报头的任何信息，读取之后忽略掉
 */
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  //空文本行终止请求报头，碰到 空行 就结束 空行后面是内容实体
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/*
 * 解析URI 为 filename 和 CGI 参数
 * 如果是动态内容返回0；静态内容返回 1
 */
int parse_uri(char *uri, char *filename, char *cgiargs) {
  if (!strstr(uri,
              "cgi-bin")) { //默认可执行文件都放在cgi-bin下，这里表示没有找到
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    /*
    if(uri[strlen(uri)-1] == "/")  //设置默认文件
        strcat(filename, "index.html");
    */

    return 1; // static
  } else {    //动态内容
    char *ptr = strchr(uri, '?');
    if (ptr) { //有参数
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } else { //无参数
      strcpy(cgiargs, "");
    }

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

/*
 * 功能：发送一个HTTP响应，主体包含一个本地文件的内容
 */
void serve_static(int fd, char *filename, int filesize) {
  int srcfd;
  char *srcp, body[MAXBUF], filetype[MAXLINE];

  /* 发送 响应行 和 响应报头 */
  get_filetype(filename, filetype);

  sprintf(body, "HTTP/1.0 200 OK\r\n");
  sprintf(body, "%sServer: Flyuz Web Server\r\n", body);
  sprintf(body, "%sConnection:close\r\n", body);
  sprintf(body, "%sContent-length: %d\r\n", body, filesize);
  sprintf(body, "%sContent-type: %s\r\n\r\n", body, filetype);
  Rio_writen(fd, body, strlen(body));
  printf("Response headers: \n%s", body);

  /* 发送响应主体 即请求文件的内容 */
  /* 只读方式发开filename文件，得到描述符*/
  srcfd = Open(filename, O_RDONLY, 0);
  /* 将srcfd 的前 filesize 个字节映射到一个从地址 srcp 开始的只读虚拟存储器区域
   * 返回被映射区的指针 */
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  /* 此后通过指针 srcp 操作，不需要这个描述符，所以关掉 */
  Close(srcfd);

  Rio_writen(fd, srcp, filesize);
  /* 释放映射的虚拟存储器区域 */
  Munmap(srcp, filesize);
}

/*
 * 功能：从文件名得出文件的类型
 */
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html") || strstr(filename, ".php"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".ipg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

/*
 * 功能：运行客户端请求的CGI程序
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE];
  char *emptylist[] = {NULL};

  /* 发送响应行 和 响应报头 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server Flyuz Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* 剩下的内容由CGI程序负责发送 */
  if (Fork() == 0) { //子进程
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, __environ);
  }
  Wait(NULL);
  /*
  if(strstr(filename, ".php")) {
            sprintf(response, "HTTP/1.1 200 OK\r\n");
            sprintf(response, "%sServer: Pengge Web Server\r\n",response);
            sprintf(response, "%sConnection: close\r\n",response);
            sprintf(response, "%sContent-type: %s\r\n\r\n",response,filetype);
            Write(connfd, response, strlen(response));
            printf("Response headers:\n");
            printf("%s\n",response);
            php_cgi(filename, connfd,cgi_params);
            Close(connfd);
            exit(0);
        //静态页面输出
  }
  */
}

/*
 * 检查一些明显的错误，报告给客户端
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  /* 构建HTTP response 响应主体 */
  sprintf(body, "<html><title>Flyuz Error</title>");
  sprintf(body,
          "%s<body bgcolor="
          "white"
          ">\r\n",
          body);
  sprintf(body, "%s<center><h1>%s: %s</h1></center>", body, errnum, shortmsg);
  sprintf(body, "%s<center><h3>%s: %s</h3></center>", body, longmsg, cause);
  sprintf(body, "%s<hr><center>The Flyuzy Web server</center>\r\n", body);

  /* 打印HTTP响应报文 */
  sprintf(buf, "HTTP/1.0 %s %s", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
