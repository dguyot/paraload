#include "utils.h"


int ticinfotime(struct infotime* nfotime)
{
	struct rusage timer;
	struct timeval rtimer;
	if ((gettimeofday(&rtimer, NULL) == 0) && (getrusage(RUSAGE_CHILDREN,&timer) == 0))
	{
		nfotime->rtime = (double)rtimer.tv_sec+((double)rtimer.tv_usec)/1000000.0;
		nfotime->utime = (double)timer.ru_utime.tv_sec+((double)timer.ru_utime.tv_usec)/1000000.0;
		nfotime->ktime = (double)timer.ru_stime.tv_sec+((double)timer.ru_stime.tv_usec)/1000000.0;
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
	double trtime;
	double tutime;
	double tktime;
	if ((gettimeofday(&rtimer, NULL) == 0) && (getrusage(RUSAGE_CHILDREN,&timer) == 0) && (nfotime->isvalid == 1))
	{
		trtime = nfotime->rtime;
		tutime = nfotime->utime;
		tktime = nfotime->ktime;
		nfotime->rtime = ((double)rtimer.tv_sec + ((double)rtimer.tv_usec)/1000000.0) - trtime;
		nfotime->utime = ((double)timer.ru_utime.tv_sec + ((double)timer.ru_utime.tv_usec)/1000000.0) - tutime;
		nfotime->ktime = ((double)timer.ru_stime.tv_sec + ((double)timer.ru_stime.tv_usec)/1000000.0) - tktime;
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
	


