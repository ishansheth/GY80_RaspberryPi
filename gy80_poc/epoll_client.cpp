#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>


#define IPADDRESS "127.0.0.1"
#define SERV_PORT 8787
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1024
#define EPOLLEVENTS 20

static void handle_connection(int sockfd);

static void handle_events(int epollfd,struct epoll_event* events,int num,int sockfd,char* buf);

static void do_read(int epollfd,int fd,int sockfd,char* buf);

static void do_write(int epollfd,int fd,int sockfd,char* buf);

static void add_event(int epollfd,int fd,int state);

static void delete_event(int epollfd,int fd,int state);

static void modify_event(int epollfd,int fd,int state);

int main(){
  int sockfd;

  struct sockaddr_in servaddr;

  sockfd = socket(AF_INET,SOCK_STREAM,0);

  bzero(&servaddr,sizeof(servaddr));

  servaddr.sin_family = AF_INET;

  servaddr.sin_port = htons(SERV_PORT);

  inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);

  connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));

  handle_connection(sockfd);

  close(sockfd);

  return 0;
}


static void handle_connection(int sockfd){
  int epollfd;

  struct epoll_event events[EPOLLEVENTS];

  char buf[MAXSIZE];

  int ret;

  epollfd = epoll_create(FDSIZE);

  add_event(epollfd,STDIN_FILENO,EPOLLIN);

  for(;;){
    printf("handle connection>>>%s",buf);

    ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);

    handle_events(epollfd,events,ret,sockfd,buf);
  }

  close(epollfd);
  
}

static
void handle_events(int epollfd,struct epoll_event *events,int num,int sockfd,char* buf){

  int fd;

  int i;

  for(i = 0;i<num;i++){
    fd = events[i].data.fd;

    if(events[i].events & EPOLLIN){
      printf("listener the some thing has written>%s",buf);
      do_read(epollfd,fd,sockfd,buf);
    }else if(events[i].events & EPOLLOUT){
      printf("listener has written some thing>>>%s",buf);
      do_write(epollfd,fd,sockfd,buf);
    }
  }
}

static void do_read(int epollfd,int fd,int sockfd,char* buf){
  int nread;

  nread = read(fd,buf,MAXSIZE);

  if(nread == -1){
    perror("read error");
    close(fd);
  }else if(nread == 0){
    fprintf(stderr,"server close");
    close(fd);
  }else{
    if(fd == STDIN_FILENO){
      printf("do read stdin: %d, %s\n",fd,buf);
      add_event(epollfd,sockfd,EPOLLOUT);
    }else{
      printf("receive the data from the other: %s\n",buf);
      delete_event(epollfd,sockfd,EPOLLIN);
      add_event(epollfd,STDOUT_FILENO,EPOLLOUT);
    }
  }
}


static void do_write(int epollfd,int fd,int sockfd,char* buf){
  int nwrite;

  nwrite = write(fd,buf,strlen(buf));

  if(nwrite == -1){
    perror("write error");
    close(fd);
  }else{
    printf("do do_write: %d, data: %s\n",fd,buf);
    if(fd == STDOUT_FILENO){
      delete_event(epollfd,fd,EPOLLOUT);
    }else{
      modify_event(epollfd,fd,EPOLLIN);
    }
  }

  memset(buf,0,MAXSIZE);
}

static void add_event(int epollfd,int fd, int state){
  struct epoll_event ev;

  ev.events = state;
  ev.data.fd = fd;

  epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

static void delete_event(int epollfd,int fd,int state){
  struct epoll_event ev;

  ev.events = state;
  ev.data.fd = fd;

  epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev); 
}


static void modify_event(int epollfd,int fd,int state){
  struct epoll_event ev;

  ev.events = state;
  ev.data.fd = fd;

  epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev); 
}


