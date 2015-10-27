#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <stdint.h>
#include <inttypes.h>

struct id_client
{
	int sock;
	struct sockaddr_in c_addr;
	char c_ip[256];
	uint32_t c_ip_int;
	uint32_t c_port;
};

struct infotime
{
	double utime;
	double ktime;
	double rtime;
	int isvalid;
};


int ticinfotime(struct infotime* nfotime);
int tacinfotime(struct infotime* nfotime);

char* internal_cmd(char* line,char* name_input,char* name_output);

char* setshmname(const char* path, const char* bsname);

char* format_cmd(char* cmd, char* tmp_in, char* tmp_out);

#endif
