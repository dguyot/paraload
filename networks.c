#include "networks.h"
#define DEBUG
#define DEBUG_LVL2
static struct epoll_event *events;
static int epoll_time_wait;
static int epoll_max_events;
static int n_events;
static int ith_event;

static int auth_flag;
static int connected_clients;
static int max_clients;

static int tcp_keepidle;
static int tcp_keepintvl;
static int tcp_keepcnt;



//
//General local tools
//


//double big/little endian conversion (host to little endian)
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
static inline double htole64_d(double le_double)
{
	union
	{
		double d;
		uint64_t uint64;
	} u;
	u.d = le_double;
	u.uint64 = htole64(u.uint64);
	return(u.d);
}


static inline double le64toh_d(double be_double)
{
	union
	{
		double d;
		uint64_t uint64;
	} u;
	u.d = be_double;
	u.uint64 = le64toh(u.uint64);
	return(u.d);
}
#endif







//return 0->OK;	1->write error;	-1->bad write len
static inline int write_sock(int sock, const void *buf, size_t size)
{
	ssize_t write_len;
	write_len = write(sock,buf,size);
	
	#ifdef DEBUG_LVL2//debug function, write on stderr all network write in hex (write are print in red)
	size_t s = 0;
	uint32_t* buf_s = (uint32_t*)buf;
	fprintf(stderr,"\033[31m");
	for(s = 0; s < (size >> 2); s++)
	{
		fprintf(stderr,"%08X(%u) ",buf_s[s],buf_s[s]);
	}
	printf("\n");
	fprintf(stderr,"\033[0m");
	#endif
	
	if (write_len == size)
	{
		return(0);
	}
	else if (write_len == -1)
	{
		return(1);
	}
	else
	{
		return(-1);
	}
}

//return 0->OK;	1->disconnection;	-1->bad read len or error
static inline int read_sock(int sock, void *buf, size_t size)
{
	ssize_t read_len;
	read_len = read(sock,buf,size);
	
	#ifdef DEBUG_LVL2//debug function, write on stderr all network read in hex (read are print in green)
	size_t s = 0;
	uint32_t* buf_s = (uint32_t*)buf;
	fprintf(stderr,"\033[32m");
	for(s = 0; s < (size >> 2); s++)
	{
		fprintf(stderr,"%08X(%u) ",buf_s[s],buf_s[s]);
	}
	printf("\n");
	fprintf(stderr,"\033[0m");
	#endif
	
	if (read_len == size)
	{
		return(0);
	}
	else if (read_len == 0)
	{
		return(1);
	}
	else
	{
		return(-1);
	}
}

//return >0 -> the length of the read -1->error
static inline ssize_t fread_sock(int sock, void *buf, size_t size)
{
	ssize_t rt;
	ssize_t rd_len = 0;
	
	while (size)
	{
		if (size >= PLD_MAX_READ_LEN)
		{
			rt = read(sock, buf, PLD_MAX_READ_LEN);
		}
		else
		{
			rt = read(sock, buf, size);
		}
	
		if (rt <= 0) return(-1);
		else
		{
			rd_len += rt;
			buf += rt;
			size -= rt;
		}
	}
	
	return(rd_len);
}

//return >0 -> the length of the write -1->error
static inline ssize_t fwrite_sock(int sock,const void *buf, size_t size)
{
	ssize_t rt;
	ssize_t wr_len = 0;

	while (size)
	{
		if (size >= PLD_MAX_WRITE_LEN)
		{
			rt = write(sock, buf, PLD_MAX_WRITE_LEN);
		}
		else
		{
			rt = write(sock, buf, size);
		}
		
		if (rt <= 0) return(-1);
		else
		{
			wr_len += rt;
			buf += rt;
			size -= rt;
		}
	}
	
	return(wr_len);
}



//
//Lonely routines only for the server or the client (no communications)
//

//the server open a listenning socket
//input: port number, backlog for the system call listen
//return <0 ->the socket to use;	-1->an error has occured
int S_getlistensock(const char* port,int backlog)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_getlistensock(%s,%i)\n",port,backlog);
	fprintf(stderr,"\033[0m");
	#endif
	int sock;
	int reuse_addr = 1;
	struct sockaddr_in local_addr;
	int number;
	
	sscanf(port, "%d", &number);
	
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return(-1);
	}
	
	if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int))) < 0)
	{
		perror("setsockopt");
		return(-1);
	}
	
	memset(&local_addr, 0, sizeof(struct sockaddr_in));

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(number);
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(local_addr.sin_zero),8);
	
	if (bind(sock, (struct sockaddr *) &local_addr, sizeof(struct sockaddr)) < 0)
	{
		close(sock);
		perror("bind");
		return(-1);
	}
	
	if (listen(sock,backlog) < 0)
	{
		perror("listen");
		return(-1);
	}
	
	return(sock);
}


//Initilization of the TCP server driver with some parameters
//input: time_wait for epoll, maxevent for epoll, auth to disable/enable authentification
//max_socket for setrlimit, idle intvl cnt: as TCP_KEEPIDLE, TCP_KEEPINTVL, TCP_KEEPCNT parameters of network stack.
//return: PLD_OK -> OK, PLD_NOK -> something bad happens
int S_init(int time_wait, int maxevents, int auth, int max_sockets, int idle, int intvl, int cnt)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_init(%i,%i,%i,%i,%i,%i,%i)\n",time_wait,maxevents,auth,max_sockets,idle,intvl,cnt);
	fprintf(stderr,"\033[0m");	
	#endif
	int rt;
	rlim_t open_limit;
	struct rlimit limit;
	
	epoll_time_wait = time_wait;
	epoll_max_events = maxevents;
	if (idle != 0) tcp_keepidle = idle;
	else tcp_keepidle = TCP_KEEPIDLE_DEFAULT;
	if (intvl != 0) tcp_keepintvl = intvl;
	else tcp_keepintvl = TCP_KEEPINTVL_DEFAULT;
	if (cnt != 0) tcp_keepcnt = cnt;
	else tcp_keepcnt = TCP_KEEPCNT_DEFAULT;
	auth_flag = auth;
	connected_clients = 0;
	n_events = 0;
	ith_event = 0;
	
	events = calloc(epoll_max_events, sizeof(struct epoll_event));
	if (events == NULL) return(PLD_NOK);
	memset(events,0,sizeof(struct epoll_event)*epoll_max_events);
	
	
	rt = getrlimit(RLIMIT_NOFILE, &limit);
	if (rt != 0) return(PLD_NOK);
	open_limit = PLD_OPENED_FD_NOTSOCKET + (rlim_t)max_sockets;
	if (open_limit > limit.rlim_max)
	{
		limit.rlim_cur = limit.rlim_max;
		open_limit = limit.rlim_max;
		fprintf(stderr, "Warning, MAX_OPEN_FILES, setted to %i the hard limit of the shell\n", (int)limit.rlim_cur);
		fprintf(stderr, "Because of this limit paraload will not accept more than %i connections\n", (int)open_limit - PLD_OPENED_FD_NOTSOCKET);
		fprintf(stderr, "You can modify it with ulimit -Hn <MAX_OPEN_FILES> as root\n\n");
	}
	else
	{
		limit.rlim_cur = open_limit;
	}
	
	max_clients = (int)open_limit - PLD_OPENED_FD_NOTSOCKET;
	
	fprintf(stderr, "Max number of clients: %i\n", max_clients);
	
	rt = setrlimit(RLIMIT_NOFILE,&limit);
	
	if (rt != 0) return(PLD_NOK);
	return(PLD_OK);
}

//free the memory of the driver
//input: nothing
//return: 0
int S_free(void)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_free(void)\n");
	fprintf(stderr,"\033[0m");	
	#endif
	if (events != NULL)
	free(events);
	return(0);
}


//wait an event with epoll system call
//input: epollfd -> an epoll set of file descriptor.
//output: a file descriptor with some activity.
//return: PLD_EPOLL_ERROR -> error or disconnection or problem with the peer
//return: PLD_EPOLL_READY -> a peer send some data on the socket
//return: PLD_EPOLL_ELAPSED -> nothing happens on the epollfd
int S_wait_event(int epollfd, int *sock_poll)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_wait_event(%i, %p)\n",epollfd,sock_poll);
	fprintf(stderr,"\033[0m");	
	#endif
	
	
	if (n_events == ith_event)
	{
		n_events = epoll_wait(epollfd, events, epoll_max_events, epoll_time_wait);
		if (n_events == -1) n_events = 0;
		ith_event = 0;
	}

	//fprintf(stderr, "%i:%i\n",ith_event,n_events);
	for(; ith_event < n_events; ith_event++)
	{
		if (events[ith_event].events & EPOLLERR)
		{
			//fprintf(stderr,"EPOLLERR\n");
			//fflush(stderr);
			*sock_poll = events[ith_event].data.fd;
			memset(&events[ith_event], 0 , sizeof(struct epoll_event));
			ith_event++;
			return(PLD_EPOLL_ERROR);
		}
		else if (events[ith_event].events & EPOLLRDHUP)
		{
			//fprintf(stderr,"EPOLLRDHUP\n");
			//fflush(stderr);
			*sock_poll = events[ith_event].data.fd;
			memset(&events[ith_event], 0 , sizeof(struct epoll_event));
			ith_event++;
			return(PLD_EPOLL_ERROR);
		}
		else if (events[ith_event].events & EPOLLHUP)
		{
			//fprintf(stdout,"EPOLLHUP\n");
			//fflush(stdout);
			*sock_poll = events[ith_event].data.fd;
			memset(&events[ith_event], 0 , sizeof(struct epoll_event));
			ith_event++;
			return(PLD_EPOLL_ERROR);
		}
		else
		{
			//fprintf(stdout,"EPOLLIN\n");
			//fflush(stdout);
			*sock_poll = events[ith_event].data.fd;
			memset(&events[ith_event], 0 , sizeof(struct epoll_event));
			ith_event++;
			return(PLD_EPOLL_READY);
		}
	}
	//fprintf(stderr,"END\n");
	return(PLD_EPOLL_ELAPSED);
}



//add new sock on epollfd
//input: epollfd -> an epoll set of file descriptor, sock -> a socket to add to the epoll set
//output: nothing
//return : PLD_OK->OK	PLD_NOK->failed to add the socket on epollfd
int S_epolladd(int epollfd, int sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_epolladd(%i, %i)\n",epollfd,sock);
	fprintf(stderr,"\033[0m");	
	#endif
	struct epoll_event event;
	memset(&event,0,sizeof(struct epoll_event));

	event.data.fd = sock;
	event.events = EPOLLIN | EPOLLPRI | EPOLLHUP | EPOLLERR;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &event) == -1)
	{
		perror("S_epolladd: epoll_clt");
		close(sock);
		return(PLD_NOK);
	}
	else
	{
		connected_clients++;
		//fprintf(stderr, "%i\n\n",connected_clients);
		return(PLD_OK);
	}
}

//delete new sock on epollfd
//input: epollfd -> an epoll set of file descriptor, sock -> a socket to delete to the epoll set
//output: nothing
//return : PLD_OK->OK	PLD_NOK->failed to delete the socket on epollfd

int S_epolldel(int epollfd, int sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_epolldel(%i, %i)\n",epollfd,sock);
	fprintf(stderr,"\033[0m");	
	#endif
	struct epoll_event event;
	memset(&event,0,sizeof(struct epoll_event));
	
	event.data.fd = sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, &event) == -1)
	{
		perror("S_epolldel: epoll_clt");
		return(PLD_NOK);
	}
	else
	{
		connected_clients--;
		close(sock);
		return(PLD_OK);
	}
}


//
//Network routines, alternate server function with client function to debug more easily.
//Client function begins with a C_ and Serveur functions begins with a S_
//Functions comes in the logical order off call
//



//first : the client make a request of connexion he needs a host and a port in tcp context

//client ask for connexion on the server
//input: host -> the ip or the name of the server host
//input: port -> the listenning port of the server
//output: a socket that can be used by the client to communicate with the server
//return: PLD_OK -> the socket is ready, PLD_NOK -> the socket is not ready.
int C_connect(const char* host, const char* port, int* sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_connect(%s, %s, %p)\n",host,port,sock);
	fprintf(stderr,"\033[0m");	
	#endif
	int tcp_delay = 1;//disable nagle algorithm
	int rt;
	struct addrinfo * addrinfos;
	struct addrinfo * addrinfo;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;//IP V4
	hints.ai_socktype = SOCK_STREAM;//TCP Connection
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	
	rt = getaddrinfo(host, port, &hints, &addrinfos);//DNS resolution

	if (rt != 0)
	{
		if (rt == EAI_SYSTEM)
		{
			perror("getaddrinfo");
		}
		else
		{
			fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(rt));
		}
		return(PLD_NOK);
	}

	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket in request_connexion");
		return(PLD_NOK);
	}
	if (setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (void *)&tcp_delay, sizeof(int)))
	{
		perror("setsockopt in request_connexion");
		close(*sock);
		return(PLD_NOK);
	}

	for(addrinfo = addrinfos; addrinfo != NULL; addrinfo = addrinfo->ai_next)
	{
		rt = connect(*sock, addrinfo->ai_addr, sizeof (struct sockaddr_in));
		if (rt == -1) perror("connect");
		else break;
	}
	
	freeaddrinfo(addrinfos);
	
	if (addrinfo == NULL)
	{
		fprintf(stderr, "Unable to connect to %s:%s\n",host,port);
		return(PLD_NOK);
	}
	return(PLD_OK);
}



//The server must accept this connexion..

//wait for clients connection
//input: sock -> the listenning socket of the server
//output sock_c -> the connected socket to the client
//output client -> structure that describe the client
//return : PLD_OK -> the connected socket is ready, PLD_NOK -> the connected socket is not ready
int S_connect(int sock, int* sock_c, struct sockaddr_in *client)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_connect(%i, %p, %p)\n",sock,sock_c,client);
	fprintf(stderr,"\033[0m");	
	#endif
	int tcp_delay = 1;//disable nagle algorithm
	int tcp_keepalive = 1;//watch for living connexion each 600 seconds
	socklen_t len;
	
	len = sizeof(struct sockaddr_in);
	
	//printf("waiting for connection\n");
	if ((*sock_c = accept(sock, (struct sockaddr *) client, &len)) < 0)
	{
		perror("accept in make_connection");
		return(PLD_NOK);
	}
	if (setsockopt(*sock_c, IPPROTO_TCP, TCP_NODELAY, (void *)&tcp_delay, sizeof(tcp_delay)) < 0)
	{
		perror("setsockopt in make_connection");
		close(*sock_c);
		return(PLD_NOK);
	}
	if (setsockopt(*sock_c, SOL_SOCKET, SO_KEEPALIVE, (void *)&tcp_keepalive, sizeof(int)))
	{
		perror("setsockopt in request_connexion");
		close(*sock_c);
		return(PLD_NOK);
	}
	if (setsockopt(*sock_c, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&tcp_keepidle, sizeof(int)))
	{
		perror("setsockopt in request_connexion");
		close(*sock_c);
		return(PLD_NOK);
	}
	if (setsockopt(*sock_c, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&tcp_keepintvl, sizeof(int)))
	{
		perror("setsockopt in request_connexion");
		close(*sock_c);
		return(PLD_NOK);
	}
	if (setsockopt(*sock_c, IPPROTO_TCP, TCP_KEEPCNT, (void *)&tcp_keepcnt, sizeof(int)))
	{
		perror("setsockopt in request_connexion");
		close(*sock_c);
		return(PLD_NOK);
	}

	return(PLD_OK);
}


//deconnect a client and remove its socket to an epoll set
//input: epollfd -> an epoll set
//input: sock_poll -> the connected socket to remove
//output: nothing
//return: PLD_OK -> socket removed, PLD_NOK -> socket not removed properly
//NOT USED AT NOW!!!!
int S_deconnect(int epollfd, int sock_poll)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_deconnect(%i, %i)\n",epollfd,sock_poll);
	fprintf(stderr,"\033[0m");	
	#endif
	struct epoll_event event;
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock_poll, &event) == -1)
	{
		return(PLD_NOK);
	}
	if (close(sock_poll) == -1)
	{
		return(PLD_NOK);
	}
	return(PLD_OK);
}




//second : the client must authenticate him to the server.
//if the authentication is bad the server send a flag that correspond to a signal (kill or another)


//authentification part of the client
//input: sock -> the connected socket towards the server
//output: nothing
//return: PLD_DEC -> there is too much clients connected
//return: PLD_OK -> authentification OK
//special : may kill the client with raise
int C_auth(int sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_auth(%i)\n",sock);
	fprintf(stderr,"\033[0m");	
	#endif
	int flag;
	int rt;
	uid_t client_id;
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	client_id = (uid_t)htole32((uint32_t)getuid());
	#else
	client_id = getuid();
	#endif
	fprintf(stderr,"%d\n",client_id);
	rt = write_sock(sock,&client_id,sizeof(uid_t));
	
	if (rt != 0) return(rt);
	rt = read_sock(sock,&flag,sizeof(uid_t));
	if (rt != 0) return(rt);
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	flag = (int)le32toh((uint32_t)flag);
	#endif
	
	if (flag == PLD_SIG_STOP)
	{
		close(sock);
		fprintf(stderr,"%s\n","Client killed by the server, perhaps the server and the client have not the same UID!!");
		raise(flag);
	}
	else if (flag == PLD_TO_MANY_CLIENT)
	{
		close(sock);
		fprintf(stderr, "The server is too busy!\n");
		return(PLD_DEC);
	}

	return(PLD_OK);
}	

//authentification part of the server
//input: sock_c -> the connected socket towards the client
//output: nothing
//return: PLD_TO_MANY_CLIENT -> too many clients on the server
//return: PLD_AUTH_NOK -> bad authentification
//return: PLD_AUTH_OK -> good authentification
int S_auth(int sock_c)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_auth(%i)\n",sock_c);
	fprintf(stderr,"\033[0m");	
	#endif
	uid_t server_id;
	uid_t client_id;
	int flag;
	int rt;
	
	rt = read_sock(sock_c,&client_id,sizeof(uid_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	client_id = (uid_t)le32toh((uint32_t)client_id);
	#endif
	if (rt != 0) return(PLD_DEC);
	if (auth_flag == PLD_AUTH_ENABLED)
	{
		server_id = getuid();
	}
	else
	{
		server_id = client_id;
	}
	if (connected_clients >= max_clients)
	{
		flag = PLD_TO_MANY_CLIENT;
		#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
		flag = (int)htole32((uint32_t) flag);
		#endif
		rt = write_sock(sock_c,&flag,sizeof(int));
		if (rt != 0) return(PLD_DEC);
		close(sock_c);
		return(PLD_TO_MANY_CLIENT);
	}
	else if (client_id != server_id)
	{
		flag = PLD_SIG_STOP;
		#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
		flag = (int)htole32((uint32_t) flag);
		#endif
		rt = write_sock(sock_c,&flag,sizeof(int));
		if (rt != 0) return(PLD_DEC);
		close(sock_c);
		return(PLD_AUTH_NOK);
	}
	else
	{
		flag = PLD_SIG_OK;
		#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
		flag = (int)htole32((uint32_t) flag);
		#endif
		rt = write_sock(sock_c,&flag,sizeof(int));
		if (rt != 0) return(PLD_DEC);
		return(PLD_AUTH_OK);
	}
}

//
//The client send the action (Get a chunk for example)
//

//the client send the action he want to make
//input: sock -> connected socket towards the server
//input: action -> GET,PUT.... the action asked
//output: nothing
//return: PLD_DEC -> deconnexion to the server is needed
//return: PLD_OK -> the server is ready for the action

int C_action(int sock, int action)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_action(%i, %i)\n",sock,action);
	fprintf(stderr,"\033[0m");	
	#endif
	int rt;
	int flag;
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	int action_le = (int)htole32((uint32_t) action);
	rt = write_sock(sock,&action_le,sizeof(int));
	#else
	rt = write_sock(sock,&action,sizeof(int));
	#endif
	if (rt != 0) return(PLD_DEC);
	rt = read_sock(sock,&flag,sizeof(int));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	flag = (int)le32toh((uint32_t) flag);
	#endif
	if (rt != 0) return(PLD_DEC);
	
	if ((flag == PLD_SIG_STOP) && (action == PLD_GET))
	{
		close(sock);
		return(PLD_DEC);
	}
	return(PLD_OK);
}

//the server read the action asked by the client
//input: sock -> connected socket towards the client
//input: flag -> flag to send to the client (eventually to kill him)
//output: action -> the action the client want to make
int S_action(int sock, int flag, int* action)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_action(%i, %i, %p)\n",sock,flag,action);
	fprintf(stderr,"\033[0m");	
	#endif
	int rt;
	
	rt = read_sock(sock,action,sizeof(int));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	*action = (int)le32toh((uint32_t)(*action));
	#endif
	if (rt != 0) 
	{
		*action = PLD_DEC;
		return(rt);
	}
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	int flag_le = (int)htole32((uint32_t) flag);
	rt = write_sock(sock,&flag_le,sizeof(int));
	#else
	rt = write_sock(sock,&flag,sizeof(int));
	#endif
	if (rt != 0)
	{
		*action = PLD_DEC;
		return(rt);
	}
	if ((flag != PLD_SIG_OK) && (*action == PLD_GET))
	{
		*action = PLD_DEC;
		return(1);
	}
	return(PLD_OK);
}


//
//implementation of the GET method the server send informations first.
//using fwrite to send, fread can't be used because it make more reads than needed by the protocol
//Use of local method fread_sock
// 

//the server send data, command, and indexs
//input: sock_c -> a connected socket towards the client
//input: begin,end -> indexs of the data.
//input: cmd_size,size -> sizes of command and data.
//input: cmd, chunk -> the command and the data
//ouput: nothing
//return: PLD_NOK -> transmition error, PLD_OK -> transmition OK
int S_get(int sock_c, off_t begin, off_t end, size_t cmd_size, size_t size, const char* cmd, const char* chunk)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_get(%i, %llu, %llu, %llu, %llu, %p, %p)\n",sock_c,begin,end,cmd_size,size,cmd,chunk);
	fprintf(stderr,"\033[0m");	
	#endif
	unsigned char S_checksum[16];
	size_t write_len = 0;
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	uint64_t cmd_size_32 = (uint64_t)cmd_size;
	uint64_t size_32 = (uint64_t)size;
	#endif

	/*perhaps may be a faster version? (less system call but memory copy)
	int dsock = dup(sock_c);
	FILE* fsock = fdopen(dsock,"w");
	
	write_len += fwrite(&begin, sizeof(off_t), 1, fsock);
	write_len += fwrite(&end, sizeof(off_t), 1, fsock);
	write_len += fwrite(&cmd_size, sizeof(size_t), 1, fsock);
	write_len += fwrite(&size, sizeof(size_t), 1, fsock);
	

	if (write_len != 4)
	{
		fclose(fsock);
		return(PLD_NOK);
	}
	//fflush(fsock);
	
	write_len = fwrite(cmd, sizeof(char), cmd_size, fsock);

	if (write_len != cmd_size)
	{
		fclose(fsock);
		return(PLD_NOK);
	}
	
	write_len = fwrite(chunk, sizeof(char), size, fsock);

	if (write_len != size)
	{
		fclose(fsock);
		return(PLD_NOK);
	}
	fclose(fsock);
	*/
	MD5(S_checksum,(unsigned char*)chunk,(unsigned long)size);
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	begin = (off_t)htole64((uint64_t) begin);
	end = (off_t)htole64((uint64_t) end);
	#if __SIZEOF_SIZE_T__ == 8
	cmd_size = (off_t)htole64((uint64_t) cmd_size);
	size = (off_t)htole64((uint64_t) size);
	#else
	cmd_size_32 = htole64((uint64_t) cmd_size_32);
	size_32 = htole64((uint64_t) size_32);
	#endif
	#endif
	
	write_len = write_sock(sock_c, &begin, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &end, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	write_len = write_sock(sock_c, &cmd_size_32, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	#else
	write_len = write_sock(sock_c, &cmd_size, sizeof(size_t));
	if (write_len != 0) return(PLD_NOK);
	#endif
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	write_len = write_sock(sock_c, &size_32, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	#else
	write_len = write_sock(sock_c, &size, sizeof(size_t));
	if (write_len != 0) return(PLD_NOK);
	#endif
	write_len = write_sock(sock_c, S_checksum, 16 * sizeof(unsigned char));
	if (write_len != 0) return(PLD_NOK);
	
	write_len = fwrite_sock(sock_c, cmd, cmd_size);
	if (write_len != cmd_size) return(PLD_NOK);
	
	write_len = fwrite_sock(sock_c, chunk, size);
	if (write_len != size) return(PLD_NOK);
	
	return(PLD_OK);
}

//the client receive the command, data and indexs
//input: sock -> a connected socket towards the server
//output: begin,end -> indexs of the data.
//output: chunk_size -> size of data.
//output: cmd, chunk -> the command and the data
//return: PLD_NOK -> transmition error, PLD_OK -> transmition OK
//warning : cmd and chunk must be initialised with NULL ptr at the first call!
int C_get(int sock, off_t* begin, off_t* end, char** cmd, char** chunk, size_t* chunk_size)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_get(%i, %p, %p, %p, %p, %p)\n",sock,begin,end,cmd,chunk,chunk_size);
	fprintf(stderr,"C_get(%i, *%llu, *%llu, %p, %p, %p)\n",sock,*begin,*end,cmd,chunk,chunk_size);
	fprintf(stderr,"\033[0m");	
	#endif
	unsigned char S_checksum[16];
	unsigned char C_checksum[16];
	int rt;
	int i;
	size_t read_len = 0;
	size_t cmd_size = 0;
	size_t size = 0;
	
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	uint64_t cmd_size_32;
	uint64_t size_32;
	#endif
	
	rt = read_sock(sock, begin, sizeof(off_t));
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	*begin = (off_t)le64toh((uint64_t) *begin);
	#endif
	
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock, end, sizeof(off_t));
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	*end = (off_t)le64toh((uint64_t) *end);
	#endif
	
	if (rt != 0) return(PLD_NOK);
	
	#if __SIZEOF_SIZE_T__ != 8
	rt = read_sock(sock, &cmd_size_32, sizeof(uint64_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	cmd_size_32 = le64toh((uint64_t)cmd_size_32);
	#endif
	if (rt != 0) return(PLD_NOK);
	cmd_size = (size_t)cmd_size_32;
	#else
	rt = read_sock(sock, &cmd_size, sizeof(size_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	cmd_size = (size_t)le64toh((uint64_t)cmd_size);
	#endif
	if (rt != 0) return(PLD_NOK);
	#endif
	
	#if __SIZEOF_SIZE_T__ != 8
	rt = read_sock(sock, &size_32, sizeof(uint64_t));
	if (rt != 0) return(PLD_NOK);
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	size_32 = le64toh((uint64_t)size_32);
	#endif
	size = (size_t)size_32;
	#else
	rt = read_sock(sock, &size, sizeof(size_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	size = (size_t)le64toh((uint64_t)size);
	#endif
	if (rt != 0) return(PLD_NOK);
	#endif
	
	rt = read_sock(sock, S_checksum, 16 * sizeof(unsigned char));
	if (rt != 0) return(PLD_NOK);
	
	if (*cmd != NULL)
	{
		free(*cmd);
		*cmd = NULL;
	}
	if (*chunk != NULL)
	{
		free(*chunk);
		*chunk = NULL;
	}

	*cmd = malloc(sizeof(char) * (cmd_size + 1));
	*chunk = malloc(sizeof(char) * (size + 1));

	read_len = fread_sock(sock, *cmd, cmd_size);

	if (read_len != cmd_size) return(PLD_NOK);
	(*cmd)[read_len] = '\0';
	
	read_len = fread_sock(sock, *chunk, size);
	if (read_len != size) return(PLD_NOK);
	(*chunk)[read_len] = '\0';
	
	MD5(C_checksum,(unsigned char*) *chunk,(unsigned long)size);
	for (i = 0; i < 15; i++)
	{
		if (S_checksum[i] != C_checksum[i]) return(PLD_NOK);
	}
	
	(*chunk_size) = size;
	return(PLD_OK);
}

//
//implementation of the PUT method the client send informations first.
//

//the client send the computed data to the server
//input: sock -> a connected socket towards the server
//input: begin,end -> indexs of the data (input data not computed).
//input: rt_value -> return code of system("command")
//input: nfotime -> time information of command run
//input: size -> size data computed.
//input: data_c -> the computed data
//output: nothing
//return: PLD_NOK -> transmition error, PLD_OK -> transmition OK
int C_put(int sock, off_t begin, off_t end, int rt_value, struct infotime* nfotime, size_t size,const char* data_c)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_put(%i, %llu, %llu, %i, %p, %llu, %p)\n",sock,begin,end,rt_value,nfotime,size,data_c);
	fprintf(stderr,"\033[0m");	
	#endif
	unsigned char C_checksum[16];
	size_t write_len = 0;
	
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	uint64_t size_32 = (uint64_t)size;
	#endif
	
	/*maybe faster? (less system call but memory copy)
	int dsock = dup(sock);
	FILE* fsock = fdopen(dsock,"w");
	
	write_len += fwrite(&begin, sizeof(off_t), 1, fsock);
	write_len += fwrite(&end, sizeof(off_t), 1, fsock);
	write_len += fwrite(&rt_value, sizeof(int), 1, fsock);
	write_len += fwrite(nfotime, sizeof(struct infotime), 1, fsock);
	write_len += fwrite(&size, sizeof(size_t), 1, fsock);
	
	
	if (write_len != 5)
	{
		return(PLD_NOK);
		fclose(fsock);
	}
	
	//fflush(fsock);
	
	write_len = fwrite(data_c, sizeof(char), size, fsock);
	if (write_len != size)
	{
		fclose(fsock);
		return(PLD_NOK);
	}
	fclose(fsock);
	*/
	MD5(C_checksum, (unsigned char*)data_c, (unsigned long)size);
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	begin = (off_t)htole64((uint64_t) begin);
	end = (off_t)htole64((uint64_t) end);
	rt_value = (int)htole32((uint32_t) rt_value);
	nfotime->utime = htole64(nfotime->utime);
	nfotime->ktime = htole64(nfotime->ktime);
	nfotime->rtime = htole64(nfotime->rtime);
	nfotime->isvalid = (int)htole32((uint32_t) nfotime->isvalid);
	nfotime->padding = (int)htole32((uint32_t) nfotime->padding);
	#if __SIZEOF_SIZE_T__ == 8
	size = (off_t)htole64((uint64_t) size);
	#else
	size_32 = htole64((uint64_t) size_32);
	#endif
	#endif
	
	write_len = write_sock(sock, &begin, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, &end, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, &rt_value, sizeof(int));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, nfotime, sizeof(struct infotime));
	if (write_len != 0) return(PLD_NOK);
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	write_len = write_sock(sock, &size_32, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	#else
	write_len = write_sock(sock, &size, sizeof(size_t));
	if (write_len != 0) return(PLD_NOK);
	#endif
	write_len = write_sock(sock, C_checksum, 16 * sizeof(unsigned char));
	if (write_len != 0) return(PLD_NOK);
	
	write_len = fwrite_sock(sock, data_c, size);
	if (write_len != size) return(PLD_NOK);

	return(PLD_OK);
}



//the client send the computed data to the server
//input: sock_c -> a connected socket towards the client
//output: begin,end -> indexs of the data (input data not computed).
//output: rt_value -> return code of system("command")
//output: nfotime -> time information of command run
//output: size -> size data computed.
//output: data_c -> the computed data
//return: PLD_NOK -> transmition error, PLD_OK -> transmition OK
//warning : chunk must be initialised with NULL ptr at the first call!
int S_put(int sock_c, off_t* begin, off_t* end, int* rt_value, struct infotime* nfotime, char** data_r, size_t* size_r)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_put(%i, %p, %p, %p, %p, %p, %p)\n",sock_c,begin,end,rt_value,nfotime,data_r,size_r);
	fprintf(stderr,"\033[0m");	
	#endif
	unsigned char S_checksum[16];
	unsigned char C_checksum[16];
	int rt;
	int i;
	size_t read_len = 0;
	size_t size = 0;
	
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	uint64_t size_32;
	#endif
	
	rt = read_sock(sock_c, begin, sizeof(off_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	*begin = (off_t)le64toh((uint64_t)(*begin));
	#endif
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, end, sizeof(off_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	*end = (off_t)le64toh((uint64_t)(*end));
	#endif
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, rt_value, sizeof(int));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	*rt_value = (int)le32toh((uint32_t)(*rt_value));
	#endif
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, nfotime, sizeof(struct infotime));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	nfotime->utime = le64toh(nfotime->utime);
	nfotime->ktime = le64toh(nfotime->ktime);
	nfotime->rtime = le64toh(nfotime->rtime);
	nfotime->isvalid = (int)le32toh((uint32_t) nfotime->isvalid);
	nfotime->padding = (int)le32toh((uint32_t) nfotime->padding);
	#endif
	if (rt != 0) return(PLD_NOK);
	#if __SIZEOF_SIZE_T__ != 8 //for 32-64 compatibility
	rt = read_sock(sock_c, &size_32, sizeof(uint64_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	size_32 = le64toh(size_32);
	#endif
	if (rt != 0) return(PLD_NOK);
	size = (size_t)size_32;
	#else
	rt = read_sock(sock_c, &size, sizeof(size_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	size = (size_t)le64toh((uint64_t)size);
	#endif
	if (rt != 0) return(PLD_NOK);
	#endif
	rt = read_sock(sock_c, C_checksum, 16 * sizeof(unsigned char));
	if (rt != 0) return(PLD_NOK);

	if (*data_r != NULL)
	{
		free(*data_r);
		*data_r = NULL;
	}

	*data_r = malloc(sizeof(char) * (size + 1));


	read_len = fread_sock(sock_c, *data_r, size);
	if (read_len != size) return(PLD_NOK);
	(*data_r)[read_len] = '\0';
	
	MD5(S_checksum,(unsigned char*) *data_r,(unsigned long)size);
	for (i = 0; i < 15; i++)
	{
		if (S_checksum[i] != C_checksum[i]) return(PLD_NOK);
	}
	(*size_r) = size;
	return(PLD_OK);
}

//
//Ping : to make sure that the server is alive!
//The client send an integer, the server add 1 to that integer and send it to the client
//

//the client send data to ensure server is alive
//input: sock -> a connected socket towards the server
//output: nothing
//return: PLD_OK -> server is running, PLD_NOK -> server does not work or is not alive
int C_ping(int sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_ping(%i)\n",sock);
	fprintf(stderr,"\033[0m");	
	#endif
	int rt;
	int rnd_serv;
	int rnd = RAND_MAX;
	
	while(rnd == RAND_MAX)//to make sure there is no integer overflow
	{
		srand(time(NULL));
		rnd = rand();
	}
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	rnd = (int)htole32((uint32_t)rnd);
	#endif
	rt = write_sock(sock, &rnd, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock, &rnd_serv, sizeof(int));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	rnd_serv = (int)le32toh((uint32_t)rnd_serv);
	#endif
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	rnd = (int)le32toh((uint32_t)rnd);
	#endif
	if (rt != 0) return(PLD_NOK);
	if (rnd + 1 == rnd_serv) return(PLD_OK);
	else return(PLD_NOK);
}

//the server respond to the ping of the client
//input: sock -> a connected socket towards the client
//output: nothing
//return: PLD_NOK -> transmition error, PLD_OK -> pong done
int S_pong(int sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_pong(%i)\n",sock);
	fprintf(stderr,"\033[0m");	
	#endif
	int rt;
	int rnd;
	rt = read_sock(sock, &rnd, sizeof(int));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	rnd = (int)le32toh((uint32_t)rnd);
	#endif
	if (rt != 0) return(PLD_NOK);
	rnd++;
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	rnd = (int)htole32((uint32_t)rnd);
	#endif
	rt = write_sock(sock, &rnd, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	return(PLD_OK);
}
	
//
//Info : to get some information about the server state
//


//server send information on clients and jobs
//input: sock_c -> a connected socket towards the client
//input: todo,inprogress,done,fail -> informations on jobs
//input:  nthclients,tabclients -> informations on clients
//output: nothing
//return: PLD_NOK -> transmition error, PLD_OK -> transmition ok
int S_info(int sock_c, uint64_t todo, uint64_t inprogress, uint64_t done, uint64_t fail, int nthclients, struct id_client *tabclients)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"S_info(%i, %lu, %lu, %lu, %lu, %i, %p)\n",sock_c,todo,inprogress,done,fail,nthclients,tabclients);
	fprintf(stderr,"\033[0m");	
	#endif
	int i;
	size_t write_len = 0;
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	todo = htole64(todo);
	inprogress = htole64(inprogress);
	done = htole64(done);
	fail = htole64(fail);
	nthclients = htole32(nthclients);
	#endif

	write_len = write_sock(sock_c, &todo, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &inprogress, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &done, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &fail, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &nthclients, sizeof(int));
	if (write_len != 0) return(PLD_NOK);
	
	for (i = 0; i < nthclients; i++)
	{
		write_len += fwrite_sock(sock_c, &(tabclients[i].c_addr), sizeof(struct sockaddr_in));
	}
	
	if (write_len != nthclients * sizeof(struct sockaddr_in)) return(PLD_NOK);
	else return(PLD_OK);
}

//client receive information on clients and jobs
//input: sock -> a connected socket towards the server
//output: nothing
//return: PLD_NOK -> transmition error, PLD_OK -> transmition ok
int C_info(int sock)
{
	#ifdef DEBUG
	fprintf(stderr,"\033[36m");
	fprintf(stderr,"C_info(%i)\n", sock);
	fprintf(stderr,"\033[0m");	
	#endif
	int i;
	int rt;
	uint64_t todo;
	uint64_t inprogress;
	uint64_t done;
	uint64_t fail;
	int nthclients;
	struct sockaddr_in c_addr;
	uint32_t port;
	char ip[256]; 

	rt = read_sock(sock, &todo, sizeof(uint64_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	todo = le64toh(todo);
	#endif
	if (rt != 0) return(PLD_NOK);
	printf("Todo:\t%"PRIu64"\n",todo);

	rt = read_sock(sock, &inprogress, sizeof(uint64_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	inprogress = le64toh(inprogress);
	#endif
	if (rt != 0) return(PLD_NOK);
	printf("Running:\t%"PRIu64"\n",inprogress);

	rt = read_sock(sock, &done, sizeof(uint64_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	done = le64toh(done);
	#endif
	if (rt != 0) return(PLD_NOK);
	printf("Done:\t%"PRIu64"\n",done);
	
	rt = read_sock(sock, &fail, sizeof(uint64_t));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	fail = le64toh(fail);
	#endif
	if (rt != 0) return(PLD_NOK);
	printf("Fail:\t%"PRIu64"\n",fail);
	
	rt = read_sock(sock, &nthclients, sizeof(int));
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	nthclients = le32toh(nthclients);
	#endif
	if (rt != 0) return(PLD_NOK);
	printf("Number of clients:\t%i\n",nthclients);

	for(i = 0; i < nthclients; i++)
	{
		memset(ip, 0, 256);
		rt = read_sock(sock, &c_addr, sizeof(struct sockaddr_in));
		if (rt != 0) return(PLD_NOK);
		port = (uint32_t) ntohs(c_addr.sin_port);
		memset(ip,0,256);
		sprintf(ip, "%s", inet_ntoa(c_addr.sin_addr));
		printf("%i:\t%s::%"PRIu32"\n",i , ip, port);
	}
	return(PLD_OK);
}

















