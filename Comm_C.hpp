#ifndef COMM_C_H
#define COMM_C_H

#define _FILE_OFFSET_BITS 64 //to use very large files even on 32 bits systems
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
	struct infotime nfotime;
	char* data_c;
	char* exe_cmd;
	char* tmp_in;
	char* tmp_out;
	const char* hote;
	const char* port;
	int sock;
	int fail;
	int rt_value;
	int padding;//to align class on 64 bits
	size_t chunk_size;//should be 32bits on 32 bits system
	size_t data_size;//should be 32bits on 32 bits system
	
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
