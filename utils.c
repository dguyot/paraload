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


#include "utils.h"


int ticinfotime(struct infotime* nfotime)
{
	struct rusage timer;
	struct timeval rtimer;
	if ((gettimeofday(&rtimer, NULL) == 0) && (getrusage(RUSAGE_CHILDREN,&timer) == 0))
	{
		nfotime->rtime = UINT64_C(1000000) * (uint64_t)rtimer.tv_sec + (uint64_t)rtimer.tv_usec;
		nfotime->utime = UINT64_C(1000000) * (uint64_t)timer.ru_utime.tv_sec + (uint64_t)timer.ru_utime.tv_usec;
		nfotime->ktime = UINT64_C(1000000) * (uint64_t)timer.ru_stime.tv_sec + (uint64_t)timer.ru_stime.tv_usec;
		nfotime->isvalid = 1;
	}
	else
	{
		nfotime->rtime = 0;
		nfotime->utime = 0;
		nfotime->ktime = 0;
		nfotime->isvalid = 0;
		return EXIT_FAILURE;
	}		
	return EXIT_SUCCESS;
}

int tacinfotime(struct infotime* nfotime)
{
	struct rusage timer;
	struct timeval rtimer;
	uint64_t trtime;
	uint64_t tutime;
	uint64_t tktime;
	if ((gettimeofday(&rtimer, NULL) == 0) && (getrusage(RUSAGE_CHILDREN,&timer) == 0) && (nfotime->isvalid == 1))
	{
		trtime = nfotime->rtime;
		tutime = nfotime->utime;
		tktime = nfotime->ktime;
		nfotime->rtime = UINT64_C(1000000) * (uint64_t)rtimer.tv_sec + (uint64_t)rtimer.tv_usec - trtime;
		nfotime->utime = UINT64_C(1000000) * (uint64_t)timer.ru_utime.tv_sec + (uint64_t)timer.ru_utime.tv_usec - tutime;
		nfotime->ktime = UINT64_C(1000000) * (uint64_t)timer.ru_stime.tv_sec + (uint64_t)timer.ru_stime.tv_usec - tktime;
	}
	else
	{
		nfotime->rtime = -1;
		nfotime->utime = -1;
		nfotime->ktime = -1;
		nfotime->isvalid = 0;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


char* setshmname(const char* path,const char* bsname)
{
	char* name;
	char stringpid[64];
	snprintf(stringpid,63,"%"PRIu64"_%"PRIu64,(uint64_t)getuid(),(uint64_t)getpid());
	if (path[strlen(path)-1] == '/') //if there is an / at the end of path
	{
		name = (char*)malloc((strlen(bsname)+strlen(path)+strlen(stringpid)+6)*sizeof(char));
		name = strcpy(name,path);
		name = strcat(name,bsname);
		name = strcat(name,stringpid);
		name = strcat(name,"\0");
	}
	else//there is no / at the end of path : add it!
	{
		name = (char*)malloc((strlen(bsname)+strlen(path)+strlen(stringpid)+7)*sizeof(char));
		name = strcpy(name,path);
		name = strcat(name,"/");
		name = strcat(name,bsname);
		name = strcat(name,stringpid);
		name = strcat(name,"\0");
	}
	return name;
}


char* format_cmd(char* cmd, char* tmp_in, char* tmp_out)
{
	size_t tmp_in_len = strlen(tmp_in);
	size_t tmp_out_len = strlen(tmp_out);
	size_t cmd_size = strlen(cmd);
	size_t f_cmd_size = cmd_size;
	char* f_cmd;
	
	char* b_chunkin = strstr(cmd, "#chunkin#");
	char* b_chunkout = strstr(cmd, "#chunkout#");
	char* e_chunkin = NULL;
	char* e_chunkout = NULL;
	
	
	if (b_chunkin != NULL)
	{
		e_chunkin = b_chunkin + 9;
		f_cmd_size = f_cmd_size -9 + tmp_in_len;
	}
	else
	{
		b_chunkin = cmd;
		e_chunkin = cmd;
		tmp_in_len = 0;
	}
	
	if (b_chunkout != NULL)
	{
		e_chunkout = b_chunkout + 10;
		f_cmd_size = f_cmd_size -10 + tmp_out_len;
	}
	else
	{
		b_chunkout = cmd;
		e_chunkout = cmd;
		tmp_out_len = 0;
	}
		
	
	f_cmd = malloc(sizeof(char)*(f_cmd_size + 1));
	
	memset(f_cmd,'\0',f_cmd_size + 1);
	
	if (b_chunkin < b_chunkout)
	{
		strncpy(f_cmd, cmd, b_chunkin - cmd);
		strncat(f_cmd, tmp_in, tmp_in_len);
		strncat(f_cmd, e_chunkin, b_chunkout - e_chunkin);
		strncat(f_cmd, tmp_out, tmp_out_len);
		strncat(f_cmd, e_chunkout, cmd_size);
	}
	else
	{
		strncpy(f_cmd, cmd, b_chunkout - cmd);
		strncat(f_cmd, tmp_out, tmp_out_len);
		strncat(f_cmd, e_chunkout, b_chunkin - e_chunkout);
		strncat(f_cmd, tmp_in, tmp_in_len);
		strncat(f_cmd, e_chunkin, cmd_size);
	}
	return(f_cmd);
}
	


