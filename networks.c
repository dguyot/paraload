#include "networks.h"

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

//return 0->OK;	1->write error;	-1->bad write len
static inline int write_sock(int sock,const void *buf, size_t size)
{
	ssize_t write_len;
	write_len = write(sock,buf,size);
//	printf("%i\n",size);
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
static inline int read_sock(int sock,void *buf, size_t size)
{
	ssize_t read_len;
	read_len = read(sock,buf,size);
//	printf("%i\n",size);
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

//return <0 ->the socket to use;	-1->an error has occured
int S_getlistensock(const char* port,int backlog)
{
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


//Initilization of the TCP server driver
int S_init(int time_wait, int maxevents, int auth, int max_sockets, int idle, int intvl, int cnt)
{
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
	if (rt != 0) return(rt);
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
	return(rt);
	
	return(PLD_OK);
}

int S_free(void)
{
	if (events != NULL)
	free(events);
	return(0);
}


//return : 0->Ok a socket is ready;	1->disconnetion;	-1->nothing happens (time limit elapsed)
int S_wait_event(int epollfd, int *sock_poll)
{
	//int i;
	//sigset_t mask;
	//memset(events,0,sizeof(struct epoll_event)*epoll_max_events);
	//n = epoll_pwait(epollfd, events, epoll_max_events, epoll_time_wait, &mask);
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


//
//add new sock on epollfd
//return : 0->Ok	1->Nok
int S_epolladd(int epollfd, int sock)
{
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

//
//delete sock on epollfd, and close sock
//return 0->Ok	1->Nok
int S_epolldel(int epollfd, int sock)
{
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


//return : 0->OK;	1->Not OK;
//params : out pointer to a ready sock
int C_connect(const char* host, const char* port, int* sock)
{
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

//return : 0->OK;	1->Not OK
//params ; out the ip of the client in char* and the port in integer.
int S_connect(int sock, int* sock_c, struct sockaddr_in *client)
{
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

//return : 0->OK;	1->Not OK
int S_deconnect(int epollfd, int *sock_poll)
{
	struct epoll_event event;
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, *sock_poll, &event) == -1)
	{
		return(PLD_NOK);
	}
	if (close(*sock_poll) == -1)
	{
		return(PLD_NOK);
	}
	return(PLD_OK);
}

//second : the client must authenticate him to the server.
//if the authentication is bad the server send a flag that correspond to a signal (kill or another)


//return 0->OK;	1->write error;	-1->bad length transmition
//special : may suicide with raise
int C_auth(int sock)
{
	int flag;
	int rt;
	uid_t client_id;
	client_id = getuid();
	
	rt = write_sock(sock,&client_id,sizeof(uid_t));
	if (rt != 0) return(rt);
	rt = read_sock(sock,&flag,sizeof(uid_t));
	if (rt != 0) return(rt);
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


//return : 0->auth ok;	1->disconnected;	-1->bad auth
int S_auth(int sock_c)
{
	uid_t server_id;
	uid_t client_id;
	int flag;
	int rt;
	
	rt = read_sock(sock_c,&client_id,sizeof(uid_t));
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
		rt = write_sock(sock_c,&flag,sizeof(int));
		if (rt != 0) return(PLD_DEC);
		close(sock_c);
		return(PLD_TO_MANY_CLIENT);
	}
	else if (client_id != server_id)
	{
		flag = PLD_SIG_STOP;
		rt = write_sock(sock_c,&flag,sizeof(int));
		if (rt != 0) return(PLD_DEC);
		close(sock_c);
		return(PLD_AUTH_NOK);
	}
	else
	{
		flag = PLD_SIG_OK;
		rt = write_sock(sock_c,&flag,sizeof(int));
		if (rt != 0) return(PLD_DEC);
		return(PLD_AUTH_OK);
	}
}

//
//The client send the action (Get a chunk for example)
//


//return : 0->OK;	other->NOK
//special : may suicide with raise(obsolete)
int C_action(int sock, int action)
{
	int rt;
	int flag;
	
	rt = write_sock(sock,&action,sizeof(int));
	if (rt != 0) return(PLD_DEC);
	rt = read_sock(sock,&flag,sizeof(int));
	if (rt != 0) return(PLD_DEC);
	
	if ((flag == PLD_SIG_STOP) && (action == PLD_GET))
	{
		close(sock);
		return(PLD_DEC);
		//fprintf(stderr,"%s\n","Client killed by the server");
		//raise(flag);
	}
	return(PLD_OK);
}

//return : 0->OK;	!=0 -> NOK (can destroy the connection)
//out : action
int S_action(int sock, int flag, int* action)
{
	int rt;
	
	rt = read_sock(sock,action,sizeof(int));
	if (rt != 0) 
	{
		*action = PLD_DEC;
		return(rt);
	}
	rt = write_sock(sock,&flag,sizeof(int));
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

//return : 0->OK;	1->error;
//out : nothing
int S_get(int sock_c, off_t begin, off_t end, size_t cmd_size, size_t size,const char* cmd,const char* chunk)
{
	unsigned char S_checksum[16];
	size_t write_len = 0;
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
	/*int i;
	for (i = 0; i < 16; i++)
	{
		printf ("%02x", (unsigned int) checksum[i]);
	}
	printf ("\n");*/
	write_len = write_sock(sock_c, &begin, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &end, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &cmd_size, sizeof(size_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &size, sizeof(size_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, S_checksum, 16 * sizeof(unsigned char));
	if (write_len != 0) return(PLD_NOK);
	
	write_len = fwrite_sock(sock_c, cmd, cmd_size);
	if (write_len != cmd_size) return(PLD_NOK);
	
	write_len = fwrite_sock(sock_c, chunk, size);
	if (write_len != size) return(PLD_NOK);
	
	return(PLD_OK);
}

//return : 0->OK;	1->error;
//out : begin, end, cmd, chunk
//warning : cmd and chunk must be initialised with NULL ptr at the first call!
int C_get(int sock, off_t* begin, off_t* end, char** cmd, char** chunk, size_t* chunk_size)
{
	unsigned char S_checksum[16];
	unsigned char C_checksum[16];
	int rt;
	int i;
	size_t read_len = 0;
	size_t cmd_size = 0;
	size_t size = 0;
	
	rt = read_sock(sock, begin, sizeof(off_t));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock, end, sizeof(off_t));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock, &cmd_size, sizeof(size_t));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock, &size, sizeof(size_t));
	if (rt != 0) return(PLD_NOK);
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

//return : 0->OK;	1->error;
//out : nothing
int C_put(int sock, off_t begin, off_t end, int rt_value, struct infotime* nfotime, size_t size,const char* data_c)
{
	unsigned char C_checksum[16];
	size_t write_len = 0;
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
	
	write_len = write_sock(sock, &begin, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, &end, sizeof(off_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, &rt_value, sizeof(int));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, nfotime, sizeof(struct infotime));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, &size, sizeof(size_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock, C_checksum, 16 * sizeof(unsigned char));
	if (write_len != 0) return(PLD_NOK);
	
	write_len = fwrite_sock(sock, data_c, size);
	if (write_len != size) return(PLD_NOK);

	return(PLD_OK);
}
	
//return 0->Ok 1->Nok	
//out : begin, end, rt_value, nfotime, data_r, size_r
//warning : chunk must be initialised with NULL ptr at the first call!
int S_put(int sock_c, off_t* begin, off_t* end, int* rt_value, struct infotime* nfotime, char** data_r, size_t* size_r)
{
	unsigned char S_checksum[16];
	unsigned char C_checksum[16];
	int rt;
	int i;
	size_t read_len = 0;
	size_t size = 0;

	rt = read_sock(sock_c, begin, sizeof(off_t));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, end, sizeof(off_t));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, rt_value, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, nfotime, sizeof(struct infotime));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock_c, &size, sizeof(size_t));
	if (rt != 0) return(PLD_NOK);
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
	
//return 0->Ok 1->Nok
//out : nothing
int C_ping(int sock)
{
	int rt;
	int rnd_serv;
	int rnd = RAND_MAX;
	
	while(rnd == RAND_MAX)//to make sure there is no integer overflow
	{
		srand(time(NULL));
		rnd = rand();
	}
	
	rt = write_sock(sock, &rnd, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	rt = read_sock(sock, &rnd_serv, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	if (rnd + 1 == rnd_serv) return(PLD_OK);
	else return(PLD_NOK);
}

//return 0->Ok else->Nok
//out : nothing
int S_pong(int sock)
{
	int rt;
	int rnd;
	rt = read_sock(sock, &rnd, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	rnd++;
	rt = write_sock(sock, &rnd, sizeof(int));
	if (rt != 0) return(PLD_NOK);
	return(PLD_OK);
}
	
//
//Info : to get some information about the server state
//


//return 0->Ok else->Nok
//out : nothing

int S_info(int sock_c, uint64_t todo, uint64_t inprogress, uint64_t done, int nthclients, struct id_client *tabclients)
{
	int i;
	size_t write_len = 0;
	/*maybe faster? (less system call but memory copy)
	int dsock = dup(sock_c);
	FILE* fsock = fdopen(dsock,"w");
	
	write_len += fwrite(&todo, sizeof(uint64_t), 1, fsock);
	write_len += fwrite(&inprogress, sizeof(uint64_t), 1, fsock);
	write_len += fwrite(&done, sizeof(uint64_t), 1, fsock);
	write_len += fwrite(&nthclients, sizeof(int), 1, fsock);

	if (write_len != 4)
	{
		fclose(fsock);
		return(PLD_NOK);
	}
	
	fflush(fsock);
	
	write_len = 0;
	
	for(i = 0; i < nthclients; i++)
	{
		write_len += fwrite(&(tabclients[i].c_addr), sizeof(struct sockaddr_in), 1, fsock);
	}
	
	fflush(fsock);
	fclose(fsock);
	if (write_len != nthclients) return(PLD_NOK);
	else return(PLD_OK);
	*/
	
	write_len = write_sock(sock_c, &todo, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &inprogress, sizeof(uint64_t));
	if (write_len != 0) return(PLD_NOK);
	write_len = write_sock(sock_c, &done, sizeof(uint64_t));
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


int C_info(int sock)
{
	int i;
	int rt;
	uint64_t todo;
	uint64_t inprogress;
	uint64_t done;
	int nthclients;
	struct sockaddr_in c_addr;
	uint32_t port;
	char ip[256]; 

	rt = read_sock(sock, &todo, sizeof(uint64_t));
	if (rt != 0) return(PLD_NOK);
	printf("Todo:\t%"PRIu64"\n",todo);

	rt = read_sock(sock, &inprogress, sizeof(uint64_t));
	if (rt != 0) return(PLD_NOK);
	printf("Running:\t%"PRIu64"\n",inprogress);

	rt = read_sock(sock, &done, sizeof(uint64_t));
	if (rt != 0) return(PLD_NOK);
	printf("Done:\t%"PRIu64"\n",done);

	rt = read_sock(sock, &nthclients, sizeof(int));
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

















