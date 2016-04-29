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


#include "Comm_C.hpp"
#include <cstdio>

/**
 * Constructor
 * Initialize pointer, structures
 * create temporary files names
 * get port, hostname of the server
 */

Comm_C::Comm_C(Cline* cline)
{
	cmd = NULL;
	chunk = NULL;
	data_c = NULL;
	exe_cmd = NULL;
	tmp_in = NULL;
	tmp_out = NULL;
	memset(&nfotime, 0, sizeof(nfotime));
	
	tmp_in = setshmname((cline->getcmd("shm")).c_str(),"plin_");
	tmp_out = setshmname((cline->getcmd("shm")).c_str(),"plout_");
	port = (cline->getcmd("port")).c_str();
	hote = (cline->getcmd("host")).c_str();
	fail = 0;
	
	C_connect(hote,port,&sock);
	C_auth(sock);
}

/**
 * Destructor
 * remove temporary files
 * free all allocated memory
 */

Comm_C::~Comm_C()
{
	remove(tmp_in);
	remove(tmp_out);
	if (cmd != NULL) free(cmd);
	if (chunk != NULL) free(chunk);
	if (data_c != NULL) free(data_c);
	if (exe_cmd != NULL) free(cmd);
	if (tmp_in != NULL) free(tmp_in);
	if (tmp_out != NULL) free(tmp_out);
}

//return : 0->OK 1->NOK
int Comm_C::receive()
{
	int rt;
	rt = C_action(sock,PLD_GET);
	if (rt != PLD_OK) return(rt);
	rt = C_get(sock, &idx_begin, &idx_end, &cmd, &chunk, &chunk_size);
	return rt;
}

int Comm_C::run()
{
	FILE* f_tmp_in;
	FILE* f_tmp_out;
	
	f_tmp_in = fopen(tmp_in, "w");
	
	if (fwrite(chunk, sizeof(char), chunk_size, f_tmp_in) != chunk_size)
	{
		fprintf(stderr, "Not enough space to fill %s\n",tmp_in);
		return(PLD_NOK);
	}
	
	fflush(f_tmp_in);
	fclose(f_tmp_in);
	free(chunk);
	chunk = NULL;

	exe_cmd = format_cmd(cmd, tmp_in, tmp_out);
	if (cmd != NULL) free(cmd);
	cmd = NULL;
		
	ticinfotime(&nfotime);
	rt_value = system(exe_cmd);
	tacinfotime(&nfotime);
	
	if (exe_cmd != NULL) free(exe_cmd);
	exe_cmd = NULL;
	
	if (WIFSIGNALED(rt_value) && (WTERMSIG(rt_value) == SIGINT || WTERMSIG(rt_value) == SIGQUIT))
	{
		return(PLD_NOK);
	}
	if (rt_value == 127)
	{
		fprintf(stderr, "Can't run the command %s unable to run sh\n", exe_cmd);
		return(PLD_NOK);
	}
	else if (rt_value == -1)
	{
		perror("system");
		return(PLD_NOK);
	}
	
	if (rt_value != 0) fail++;
	
	f_tmp_out = fopen(tmp_out, "r");
	
	fseek(f_tmp_out, 0, SEEK_END);
	data_size = ftell(f_tmp_out);
	fseek(f_tmp_out, 0, SEEK_SET);
	
	data_c = (char*)malloc(sizeof(char)*data_size + 1);
	
	if (fread(data_c, sizeof(char), data_size, f_tmp_out) != data_size)
	{
		fprintf(stderr, "Can't read the entire file %s\n",tmp_out);
		return(PLD_NOK);
	}
	
	data_c[data_size] = '\0';
	fclose(f_tmp_out);
	remove(tmp_out);
	
	return(PLD_OK);
}

int Comm_C::send()
{
	C_action(sock,PLD_PUT);
	C_put(sock, idx_begin, idx_end, rt_value, &nfotime, data_size, data_c);
	free(data_c);
	data_c = NULL;
	return(0);
}

int Comm_C::ping()
{
	int rt;
	C_action(sock,PLD_PING);
	rt = C_ping(sock);
	if (rt == PLD_OK)
	{
		printf("Ok server seems to be alive!\n");
	}
	else
	{
		printf("It seems that there is no server on %s::%s\n", hote, port);
	}
	return(0);
}

int Comm_C::info()
{
	int rt;
	C_action(sock,PLD_INFO);
	rt = C_info(sock);
	if (rt == PLD_OK)
	{
		printf("Ok all info retreive from the server..\n");
		return(0);
	}
	else
	{
		printf("Warning incomplete info from the server..\n");
		return(1);
	}
}

int Comm_C::fails()
{
	return(fail);
}
