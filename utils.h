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
	uint64_t utime;
	uint64_t ktime;
	uint64_t rtime;
	int isvalid;
	int padding;//to align struct on 64 bits
};


int ticinfotime(struct infotime* nfotime);
int tacinfotime(struct infotime* nfotime);

char* internal_cmd(char* line,char* name_input,char* name_output);

char* setshmname(const char* path, const char* bsname);

char* format_cmd(char* cmd, char* tmp_in, char* tmp_out);

#endif
