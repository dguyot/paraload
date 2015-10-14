#ifndef COMM_C_H
#define COMM_C_H

#include "Define.hpp"
#include "Cline.hpp"

extern "C"{
#include "utils.h"
#include "networks.h"
}




class Comm_C
{
	private:

	off_t idx_begin;
	off_t idx_end;
	char* cmd;
	char* chunk;
	size_t chunk_size;
	int rt_value;
	struct infotime nfotime;
	char* data_c;
	size_t data_size;
	char* exe_cmd;
	char* tmp_in;
	char* tmp_out;
	const char* hote;
	const char* port;
	int sock;
	int fail;

	public:
	Comm_C(Cline* cline);
	~Comm_C();
	int ping();
	int info();
	int receive();
	int run();
	int send();
	int fails();
};



#endif
