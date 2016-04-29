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


#ifndef COMM_S_H
#define COMM_S_H

#define _FILE_OFFSET_BITS 64 //to use very large files even on 32 bits systems
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
	
	struct infotime nfotime;
	
	struct id_client *tabclients;
	int nthclients;
	int rt_value;
	
	char* current_ip;
	uint32_t current_ip_int;
	uint32_t current_port;
	rlim_t max_open_files;
	int max_sockets;
	
	struct sockaddr_in c_addr;
	int padding;//to align class on 64 bits


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
