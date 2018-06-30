/*
 * @filename:    functionLib.c
 * @author:      Flyuz
 * @date:        2018年6月25日
 * @description: 辅助函数
 */

#include "functionLib.h"

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n",msg, strerror(errno));
    exit(0);
}

pid_t Fork()
{
    pid_t pid;
    pid = fork();
    if(pid < 0)
        unix_error("fork error");
    return pid;
}

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if(execve(filename, argv,envp) < 0)
        unix_error("Execve error");
}

pid_t Wait(int *status)
{
    pid_t pid;

    if((pid = wait(status)) < 0)
        unix_error("Wait error");
    return pid;
}



int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if((rc = open(pathname, flags, mode)) < 0)
        unix_error("Open error");
    return rc;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if((rc = accept(s, addr,addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}

void Close(int fd)
{
    int rc;

    if((rc = close(fd)) < 0)
        unix_error("Close error");
}

int Dup2(int oldfd, int newfd)
{
    int rc;

    if((rc = dup2(oldfd, newfd)) < 0 )
        unix_error("Dup2 error");
    return rc;
}


void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *ptr;

    if((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *)-1))
        unix_error("Mmap error");
    return ptr;
}

void Munmap(void *start, size_t length)
{
    if(munmap(start, length) < 0)
        unix_error("Munmap error");
}
/*
 * 无缓冲输入函数
 * 成功返回收入的字节数
 * 若EOF返回0 ，出错返回-1
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    char *bufp = usrbuf;
    size_t nleft = n;
    ssize_t nread;
    while(nleft > 0) {
        if((nread = read(fd,bufp,nleft)) < 0) {
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if( nread == 0 )
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n-nleft);
}
/*
 * 无缓冲输出函数
 * 成功返回输出的字节数，出错返回-1
*/
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    char *bufp = usrbuf;
    size_t nleft = n;
    ssize_t nwritten;
    while(nleft > 0) {
        if((nwritten = write(fd, bufp, nleft)) <= 0) {
            if(errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}


static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    while(rp -> rio_cnt <= 0) {
        rp -> rio_cnt = read(rp -> rio_fd, rp -> rio_buf,
                sizeof(rp -> rio_buf));
        if(rp -> rio_cnt < 0) {
            if(errno != EINTR)
                return -1;
        } else if(rp -> rio_cnt == 0)
            return 0;
        else
            rp -> rio_bufp = rp -> rio_buf;
    }

    int cnt = rp -> rio_cnt < n ? rp -> rio_cnt : n;

    memcpy(usrbuf, rp -> rio_bufp, cnt);
    rp -> rio_bufp += cnt;
    rp -> rio_cnt -= cnt;
    return cnt;
}
// 初始化rio_t结构，创建一个空的读缓冲区
// 将fd和地址rp处的这个读缓冲区联系起来
void rio_readinitb(rio_t *rp, int fd)
{
    rp -> rio_fd = fd;
    rp -> rio_cnt = 0;
    rp -> rio_bufp = rp -> rio_buf;
}

//带缓冲输入函数
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while(nleft > 0) {
        if((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1;
        else if (nread == 0)
            break;

        nleft -= nread;
        bufp += nread;
    }
    return (n-nleft);
}
/*
**带缓冲输入函数，每次输入一行
**从文件rp读出一个文本行(包括结尾的换行符)，将它拷贝到usrbuf，并且用空字符来结束这个文本行
**最多读maxlen-1个字节，余下的一个留给结尾的空字符
**/
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;
    for(n = 1; n<maxlen; n++) {
        if((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if( c == '\n' ){
                n++;
                break;
            }
        } else if(rc == 0) {
            if( n  == 1 )
                return 0;
            else
                break;
        } else
            return -1;
    }
    *bufp = '\0';
    return n-1;
}

ssize_t Rio_readn(int fd, void *usrbuf, size_t n)
{
    ssize_t nbytes;
    if((nbytes = rio_readn(fd, usrbuf, n)) < 0)
        unix_error("Rio_readn error");

    return nbytes;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    if(rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t nbytes;
    if((nbytes = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return nbytes;
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t nbytes;
    if((nbytes = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb error");
    return nbytes;
}

/*打开并返回监听描述符*/
int open_listenfd(char *port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                (const void *)&optval, sizeof(int)) < 0)
        return -1;

    bzero((char *)&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)atoi(port));

    if(bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    if(listen(listenfd, LISTENQ) < 0)
        return -1;

    return listenfd;
}

int Open_listenfd(char *port)
{
    int rc;
    if((rc = open_listenfd(port)) < 0)
        unix_error("open_listenfd error");
    return rc;
}
//信号处理
handler_t  *signal_r(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(signum, &action, &old_action) < 0)
	      perror("Signal error");
    return (old_action.sa_handler);
}
//排队 防止阻塞
void sigchild_handler(int sig)
{
    int stat;
	  while(waitpid(-1,&stat,WNOHANG)>0);
	  return;
}
