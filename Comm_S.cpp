#include "Comm_S.hpp"
#include "Define.hpp"
#include <iostream>

extern "C"{
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
}

using namespace::std;

Comm_S::Comm_S(Conf* conf, Cline* cline)
{
	struct epoll_event event;
	int auth;
	int epoll_max_events;
	int epoll_time_wait;
	int max_sockets;
	int tcp_keepidle;
	int tcp_keepintvl;
	int tcp_keepcnt;
	
	
	
	tabclients = NULL;
	nthclients = 0;
	
	data_r = NULL;

	signal_flag = PLD_SIG_OK;

	cmd = new char[LG_BUFFER];
	memset(cmd,0,LG_BUFFER);
	strncpy(cmd, (conf->getconf("cmd")).c_str(), LG_BUFFER);
	cmd_size = strlen(cmd);
	
	max_open_files = (rlim_t)atoi(conf->getconf("max_open_files").c_str());
	//setopenlimit(max_open_files);
	
	//first create the epoll descriptor
	epoll_fd = epoll_create(1);
	if (epoll_fd == -1)
	{
		perror("syscall epoll_create in Comm_S");
		exit(EXIT_FAILURE);
	}

	//ok now create a listening socket interface
	sock_s = S_getlistensock((cline->getcmd("port")).c_str(),atoi(conf->getconf("listen_backlog").c_str()));
	
	memset(&event,0,sizeof(struct epoll_event));
	event.data.fd = sock_s;
	event.events = EPOLLIN | EPOLLPRI | EPOLLHUP | EPOLLERR;
	
	if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, sock_s, &event) == -1)
	{
		perror("syscall epoll_ctl in Comm_S");
		exit(EXIT_FAILURE);
	}
	if (conf->getconf("authentication") == "NO")
	{
		auth = PLD_AUTH_DISABLED;
	}
	else
	{
		auth = PLD_AUTH_ENABLED;
	}
	epoll_max_events = atoi(conf->getconf("epoll_max_events").c_str());
	epoll_time_wait = atoi(conf->getconf("epoll_time_wait").c_str());
	max_sockets = atoi(conf->getconf("max_sockets").c_str());
	tcp_keepidle = atoi(conf->getconf("tcp_keepidle").c_str());
	tcp_keepintvl = atoi(conf->getconf("tcp_keepintvl").c_str());
	tcp_keepcnt = atoi(conf->getconf("tcp_keepcnt").c_str());
	
	
	
	S_init(epoll_time_wait, epoll_max_events, auth, max_sockets, tcp_keepidle, tcp_keepintvl, tcp_keepcnt);
	//S_init(atoi(conf->getconf("epoll_time_wait").c_str()), atoi(conf->getconf("epoll_max_events").c_str()), auth, atoi(conf->getconf("max_sockets").c_str()));
}

Comm_S::~Comm_S()
{
	delete[] cmd;
	if (tabclients != NULL) free(tabclients);
	if (data_r != NULL) free(data_r);
	S_free();
}

int Comm_S::wait()
{
	int i;
	int sock_poll;
	int rtevent;
	int action;
	int rt;
	
	while ((rtevent = S_wait_event(epoll_fd,&sock_poll)) != -1)
	{
		//store current client info:
		
		if ((rtevent != PLD_EPOLL_ELAPSED) && (sock_poll != sock_s))
		{
			for (i = 0; i < nthclients; i++)
			{
				if (tabclients[i].sock == sock_poll)
				{
					c_addr = tabclients[i].c_addr;
					current_port = tabclients[i].c_port;
					current_ip_int = tabclients[i].c_ip_int;
					current_ip = tabclients[i].c_ip;
					break;
				}
			}
		}
		
		if (rtevent == PLD_EPOLL_READY)
		{
			if (sock_poll == sock_s)
			{
				if (S_connect(sock_poll, &sock_c, &c_addr) == PLD_OK)
				{
					rt = S_auth(sock_c);
					if (rt == PLD_AUTH_OK)
					{
						if (S_epolladd(epoll_fd,sock_c) == PLD_OK)
						{
							return(PLD_CON);
						}
					}
					else if (rt == PLD_TO_MANY_CLIENT)
					{
						//fprintf(stderr,"Client rejected : to many clients\n");
					} 
					else if (rt == PLD_AUTH_NOK)
					{
						//fprintf(stderr,"Comm_S::wait bad auth\n");
					}
				}
				else
				{
					fprintf(stderr,"Comm_S::wait connection failed\n");
				}
			}
			else
			{
				sock_c = sock_poll;
				if (S_action(sock_c, signal_flag, &action) != PLD_OK)
				{
					S_epolldel(epoll_fd,sock_c);
					return(PLD_DEC);
				}
				return(action);
			}
		}
		else if (rtevent == PLD_EPOLL_ERROR)
		{
			sock_c = sock_poll;
			S_epolldel(epoll_fd, sock_c);
			return(PLD_DEC);
		}
	}
	return(PLD_CONT);
}


int Comm_S::send(off_t idx_begin, off_t idx_end, size_t size, const char* chunk)
{
	return(S_get(sock_c, idx_begin, idx_end, cmd_size, size, cmd, chunk));
}

int Comm_S::receive()
{
	return(S_put(sock_c, &idx_begin, &idx_end, &rt_value, &nfotime, &data_r, &size_r));
}

int Comm_S::pong()
{
	return(S_pong(sock_c));
}

int Comm_S::set_signal_flag(int signal)
{
	signal_flag = signal;
	return(0);
}

int Comm_S::get_signal_flag()
{
	return(signal_flag);
}

char* Comm_S::getres()
{
	return(data_r);
}

off_t Comm_S::get_idx_begin()
{
	return idx_begin;
}

off_t Comm_S::get_idx_end()
{
	return idx_end;
}

int Comm_S::get_rt_value()
{
	return(rt_value);
}

char* Comm_S::get_ip()
{
	return(current_ip);
}

uint32_t Comm_S::get_ip_int()
{
	return(current_ip_int);
}

uint32_t Comm_S::get_port()
{
	return(current_port);
}

double Comm_S::get_rtime()
{
	return(nfotime.rtime);
}

double Comm_S::get_utime()
{
	return(nfotime.utime);
}

double Comm_S::get_ktime()
{
	return(nfotime.ktime);
}

int Comm_S::get_nb_clients()
{
	return(nthclients);
}

int Comm_S::add_client()
{
	if (nthclients < 0)
	{
		return(-1);
	}
	if (nthclients == 0)
	{
		tabclients = (struct id_client *)malloc(sizeof(struct id_client));
	}
	else
	{
		tabclients = (struct id_client *)realloc(tabclients, (1 + nthclients) * sizeof(struct id_client));
	}
	if (tabclients == NULL)	return(-1);
	memset(&(tabclients)[nthclients],0,sizeof(struct id_client));
	tabclients[nthclients].sock = sock_c;
	tabclients[nthclients].c_addr = c_addr;
	tabclients[nthclients].c_port = (uint32_t)ntohs(c_addr.sin_port);
	current_port = tabclients[nthclients].c_port;
	tabclients[nthclients].c_ip_int	= ntohl(c_addr.sin_addr.s_addr);
	current_ip_int = tabclients[nthclients].c_ip_int;
	memset(tabclients[nthclients].c_ip,0,256);
	sprintf(tabclients[nthclients].c_ip, "%s", inet_ntoa(c_addr.sin_addr));
	current_ip = tabclients[nthclients].c_ip;
	nthclients++;
	return 0;
}

int Comm_S::remove_client()
{
	int i;

	for (i = 0; i < nthclients; i++)
	{
		if (sock_c == (tabclients)[i].sock) break;
	}
	nthclients--;
	if (i != nthclients)
	{
		memset(&(tabclients)[i],0,sizeof(struct id_client));
		(tabclients)[i].sock = (tabclients)[nthclients].sock;
		(tabclients)[i].c_addr = (tabclients)[nthclients].c_addr;
		(tabclients)[i].c_port = (tabclients)[nthclients].c_port;
		(tabclients)[i].c_ip_int = (tabclients)[nthclients].c_ip_int;
		strncpy((tabclients)[i].c_ip,(tabclients)[nthclients].c_ip, 256);
	}
	if (nthclients > 0)
	{
		tabclients = (struct id_client *)realloc(tabclients, (1 + nthclients) * sizeof(struct id_client));
	}
	else
	{
		free(tabclients);
		tabclients = NULL;
	}
	return(0);
}

int Comm_S::info(uint64_t todo, uint64_t inprogress, uint64_t done, uint64_t fail)
{
	int rt;
	rt = S_info(sock_c, todo, inprogress, done, fail, nthclients, tabclients);
	return(rt);
}
