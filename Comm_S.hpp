#ifndef COMM_S_H
#define COMM_S_H

#include "Cline.hpp"
#include "Conf.hpp"

extern "C"{
#include "utils.h"
#include "networks.h"
#include <sys/time.h>
#include <sys/resource.h>
}

class Comm_S
{
	private:
	int sock_s;
	int sock_c;
	int epoll_fd;
	//int epoll_max_events;
	//int epoll_time_wait;

	int signal_flag;

	char* cmd;
	size_t cmd_size;
	
	char* data_r;
	size_t size_r;
	
	
	off_t idx_begin;
	off_t idx_end;
	int rt_value;
	struct infotime nfotime;
	
	struct id_client *tabclients;
	int nthclients;
	
	char* current_ip;
	uint32_t current_ip_int;
	uint32_t current_port;
	rlim_t max_open_files;
	int max_sockets;
	
	struct sockaddr_in c_addr;

	public:
	Comm_S(Conf* conf,Cline* cline);
	~Comm_S();
	int wait();
	int send(off_t begin, off_t end, size_t size, const char* chunk);
	int receive();
	int pong();
	int info(uint64_t todo, uint64_t inprogress, uint64_t done, uint64_t fail);
	int set_signal_flag(int signal);
	int get_signal_flag();
	char* getres();
	off_t get_idx_begin();
	off_t get_idx_end();
	int get_rt_value();
	char* get_ip();
	uint32_t get_ip_int();
	uint32_t get_port();
	double get_rtime();
	double get_utime();
	double get_ktime();
	
	int add_client();
	int remove_client();

	int get_nb_clients();
};




#endif
