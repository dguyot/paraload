#ifndef NETWORKS_H
#define NETWORKS_H


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "utils.h"
#include "md5.h"

#define TCP_KEEPIDLE_DEFAULT 7200
#define TCP_KEEPINTVL_DEFAULT 75
#define TCP_KEEPCNT_DEFAULT 9

#define PLD_MAX_READ_LEN 4096
#define PLD_MAX_WRITE_LEN 4096
#define PLD_OPENED_FD_NOTSOCKET 24

#define PLD_SIG_OK 0
#define PLD_TO_MANY_CLIENT 1
#define PLD_SIG_STOP 15
#define PLD_OK 0
#define PLD_NOK 1
#define PLD_GET 2
#define PLD_PUT 3
#define PLD_CON 4
#define PLD_DEC 5
#define PLD_CONT 6
#define PLD_PING 7
#define PLD_INFO 8

#define PLD_AUTH_ENABLED 1
#define PLD_AUTH_DISABLED 0

#define PLD_AUTH_OK 0
#define PLD_AUTH_NOK -1

#define PLD_EPOLL_READY 0
#define PLD_EPOLL_ERROR 1
#define PLD_EPOLL_ELAPSED -1


int S_getlistensock(const char* port,int backlog);

int S_wait_event(int epollfd, int *sock_poll);

int S_epolladd(int epollfd, int sock);
int S_epolldel(int epollfd, int sock);

int S_init(int time_wait, int maxevents, int auth, int max_sockets, int idle, int intvl, int cnt);
int S_free(void);

int C_connect(const char* host, const char* port, int* sock);
int S_connect(int sock, int* sock_c, struct sockaddr_in *client);
int S_deconnect(int epollfd, int *sock_poll);

int C_auth(int sock);
int S_auth(int sock_c);

int C_action(int sock, int action);
int S_action(int sock, int flag, int* action);

int S_get(int sock_c, off_t begin, off_t end, size_t cmd_size, size_t size,const char* cmd,const char* chunk);
int C_get(int sock, off_t* begin, off_t* end, char** cmd, char** chunk, size_t* chunk_size);

int C_put(int sock, off_t begin, off_t end, int rt_value, struct infotime* nfotime, size_t size,const char* data_c);
int S_put(int sock_c, off_t* begin, off_t* end, int* rt_value, struct infotime* nfotime, char** data_c, size_t* size_r);

int C_ping(int sock);
int S_pong(int sock);

int S_info(int sock_c, uint64_t todo, uint64_t inprogress, uint64_t done, int nthclients, struct id_client *tabclients);
int C_info(int sock);


#endif
