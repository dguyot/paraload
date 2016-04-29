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


#include "Conf.hpp"
#include <iostream>
#include <fstream>
#include <sstream>


using namespace::std;

Conf::Conf(string filename,char separator)
{
	string line;
	string field_type;
	string field_value;
	ifstream conf_file(filename.c_str());
	while(getline(conf_file,line))
	{
		istringstream sline(line);
		getline(sline,field_type,separator);
		getline(sline,field_value);
		//test if the first caracter is #
		if (field_type.substr(0,1) != "#")
		{
			//remove space and tabulations in the beginning of value
			while (field_value.find(" ") == 0 || field_value.find("\t") == 0)
			{
				field_value = field_value.substr(1,string::npos);
			}
			//retrieve field type ans store value
			if (field_type.find("PRGM") == 0)
				cmd = field_value;
			else if (field_type.find("SENTINEL") == 0)
				sentinel = field_value;
			else if (field_type.find("POLICY_LIMIT") == 0)
				policy_limit = field_value;
			else if (field_type.find("POLICY") == 0)
				policy = field_value;
			else if (field_type.find("ROUND") == 0)
				round = field_value;
			else if (field_type.find("AUTHENTICATION") == 0)
				authentication = field_value;
			else if (field_type.find("LISTEN_BACKLOG") == 0)
				listen_backlog = field_value;
			else if (field_type.find("EPOLL_TIME_WAIT") == 0)
				epoll_time_wait = field_value;
			else if (field_type.find("EPOLL_MAX_EVENTS") == 0)
				epoll_max_events = field_value;
			else if (field_type.find("MAX_SOCKETS") == 0)
				max_sockets = field_value;
			else if (field_type.find("TCP_KEEPIDLE") == 0)
				tcp_keepidle = field_value;
			else if (field_type.find("TCP_KEEPINTVL") == 0)
				tcp_keepintvl = field_value;
			else if (field_type.find("TCP_KEEPCNT") == 0)
				tcp_keepcnt = field_value;
		}
	}
	voidstring = "";
}



const string& Conf::getconf(string what)
{
	if (what == "cmd")
	return cmd;
	if (what == "sentinel")
	return sentinel;
	else if (what == "policy")
	return policy;
	else if (what == "policy_limit")
	return policy_limit;
	else if (what == "round")
	return round;
	else if (what == "authentication")
	return authentication;
	else if (what == "listen_backlog")
	return listen_backlog;
	else if (what == "epoll_time_wait")
	return epoll_time_wait;
	else if (what == "epoll_max_events")
	return epoll_max_events;
	else if (what == "max_sockets")
	return max_sockets;
	else if (what == "tcp_keepidle")
	return tcp_keepidle;
	else if (what == "tcp_keepintvl")
	return tcp_keepintvl;
	else if (what == "tcp_keepcnt")
	return tcp_keepcnt;
	else
	return voidstring;
}
