#ifndef CONF_H
#define CONF_H

#include <string>


class Conf
{
	private:
	std::string cmd;
	std::string sentinel;
	std::string policy;
	std::string policy_limit;
	std::string round;
	std::string authentication;
	std::string listen_backlog;
	std::string epoll_time_wait;
	std::string epoll_max_events;
	std::string max_sockets;
	std::string	tcp_keepidle;
	std::string	tcp_keepintvl;
	std::string	tcp_keepcnt;
	std::string voidstring;
	public:
	Conf(std::string filename,char separator);
	const std::string& getconf(std::string what);
};

#endif
