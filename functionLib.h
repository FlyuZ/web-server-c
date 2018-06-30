/*
 * @filename:    functionLib.h
 * @author:      Flyuz
 * @date:        2018年6月25日
 * @description: 函数实现
 */

#ifndef ALL_H
#define ALL_H

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
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define RIO_BUFSIZE 8192
#define	MAXLINE	 8192  /* 一行最大长度 */
typedef struct {
    int rio_fd;                 //内部读缓冲区描述符
    int rio_cnt;                //内部缓冲区中未读字节数
    char *rio_bufp;           //内部缓冲区中下一个未读的字节
    char rio_buf[RIO_BUFSIZE];  //内部读缓冲区
}rio_t;                         //一个类型为rio_t的读缓冲区

#define     MAXLINE     8192
#define     MAXBUF      8192
#define     LISTENQ     1024

typedef struct sockaddr SA;
typedef void handler_t(int);
void unix_error(char *msg);

/* Process control wrappers */
pid_t Fork();
void Execve(const char *filename, char *const argv[], char *const envp[]);
pid_t Wait(int *status);

/* Unix I/O */
int Open(const char *pathname, int flags, mode_t mode);
void Close(int fd);
int Dup2(int oldfd, int newfd);



void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);

/* RIO package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);


ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
void Rio_writen(int fd, void *usrbuf, size_t n);
void Rio_readinitb(rio_t *rp, int fd);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);


int open_listenfd(char *port);

int Open_listenfd(char *port);

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);

handler_t  *signal_r(int signum, handler_t *handler);
void sigchild_handler(int sig);

#endif
