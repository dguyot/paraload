/*

Copyright or © or Copr. Dominique GUYOT 2016

dominique.guyot@univ-lyon1.fr

This software is a computer program whose purpose is to make load balancing
for very large data parallel computing. 

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.

*/

#ifndef NETWORKS_H
#define NETWORKS_H

#define _FILE_OFFSET_BITS 64 //to use very large files even on 32 bits systems

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
int S_deconnect(int epollfd, int sock_poll);

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

int S_info(int sock_c, uint64_t todo, uint64_t inprogress, uint64_t done, uint64_t fail, int nthclients, struct id_client *tabclients);
int C_info(int sock);


#endif
