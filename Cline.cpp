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




#include "Cline.hpp"
#include <getopt.h>
#include <iostream>
#include <fstream>

using namespace::std;

Cline::Cline(int argc,char **argv)
{
	int c;
	int option_index = 0;
 	struct option opt[21] =
    	{
		{"input", 1, 0, (int)('i')},
		{"output", 1, 0, (int)('o')},
		{"conf", 1, 0, (int)('C')},
		{"host", 1, 0, (int)('h')},
		{"port", 1, 0, (int)('p')},
		{"log", 1, 0, (int)('l')},
		{"shm", 1, 0, (int)('S')},
		{"server", 0, 0, (int)('s')},
		{"client", 0, 0, (int)('c')},
		{"watch", 0, 0, (int)('w')},
		{"verbose", 0, 0, (int)('v')},
		{"help", 0, 0, (int)('?')},
		{"version", 0, 0, (int)('V')},
		{"ping", 0, 0, (int)('P')},
		{"info", 0, 0, (int)('I')},
		{"bg", 0, 0, (int)('b')},
		{"report", 1, 0, (int)('r')},
		{"fails", 1, 0, (int)('f')},
		{"tool", 1, 0, (int)('t')},
		{"outsort", 1, 0, (int)('O')},
		{0, 0, 0, 0},
	};

	input = "";
	output = "";
	conf = "paraload.conf";
	log = "";
	host = "localhost";
	port = "";
	server = "no";
	client = "no";
	watch = "no";
	verbose = "no";
	shm = "/dev/shm";
	help = "no";
	version = "no";
	ping = "no";
	info = "no";
	bg = "no";
	report = "";
	fails = "-1";
	voidstring = "";
	tool = "";
	outsort = "";


	while( (c = getopt_long(argc,argv,"i:o:C:h:p:l:S:scwv?VPIbr:f:t:O:",opt,&option_index)) != -1)
    	{
		switch(c)
		{
			case 'i':
				input = optarg;
				break;
			case 'o':
				output = optarg;
				break;
			case 'C':
				conf = optarg;
				break;
			case 'h':
				host = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case 'l':
				log = optarg;
				break;
			case 'S':
				shm = optarg;
				break;
			case 's':
				server = "yes";
				break;
			case 'c':
				client = "yes";
				break;
			case 'w':
				watch = "yes";
				break;
			case 'v':
				verbose = "yes";
				break;
			case '?' :
				help = "yes";
				break;
			case 'V' :
				version = "yes";
				break;
			case 'P' :
				ping = "yes";
				break;
			case 'I' :
				info = "yes";
				break;
			case 'b' :
				bg = "yes";
				break;
			case 'r' :
				report = optarg;
				break;
			case 'f' :
				fails = optarg;
				break;
			case 't' :
				tool = optarg;
				break;
			case 'O' :
				outsort = optarg;
				break;
		}
	}
}


const string& Cline::getcmd(string what)
{
	if (what == "input")
	return input;
	else if (what == "output")
	return output;
	else if (what == "conf")
	return conf;
	else if (what == "host")
	return host;
	else if (what == "port")
	return port;
	else if (what == "log")
	return log;
	else if (what == "shm")
	return shm;
	else if (what == "server")
	return server;
	else if (what == "client")
	return client;
	else if (what == "watch")
	return watch;
	else if (what == "verbose")
	return verbose;
	else if (what == "help")
	return help;
	else if (what == "version")
	return version;
	else if (what == "ping")
	return ping;
	else if (what == "info")
	return info;
	else if (what == "bg")
	return bg;
	else if (what == "report")
	return report;
	else if (what == "fails")
	return fails;
	else if (what == "tool")
	return tool;
	else if (what == "outsort")
	return outsort;
	else
	return voidstring;
}

int Cline::check()
{
	int port_number;
	ifstream config;
	ifstream in;
	ifstream ou;
	ifstream lo;
	
	
	if (help == "yes")
	{
		help_func();
		return(1);
	}
	if (version == "yes")
	{
		version_func();
		return(1);
	}
	
	if ((client == "yes") && (server == "yes"))
	{
		cerr << "Make a choice: run a server with [--server] [-s] or a client with [--client] [-c]" << endl;
		cerr << "More help with --help or -?" << endl;
		return(1);
	}
	if ((client == "no") && (server == "no") && (tool == ""))
	{
		cerr << "Make a choice: run a server with [--server] [-s] or a client with [--client] [-c]" << endl;
		cerr << "More help with --help or -?" << endl;
		return(1);
	}
	if (server == "yes")
	{
		if (port == "")
		{
			cerr << "Please specify	a port [--port] [-p]" << endl;
			return(1);
		}
		if (input == "")
		{
			cerr << "Please specify an input [--input] [-i]" << endl;
			return(1);
		}
		in.open(input.c_str());
		if (in.fail())
		{
			cerr << "Unable to find or read input file [--input] [-i]" << endl;
			return(1);
		}
		if (output == "")
		{
			cerr << "Please specify an output [--output] [-o]" << endl;
			return(1);
		}
		if (log == "")
		{
			cerr << "Please specify a log [--log] [-l]" << endl;
			return(1);
		}
		config.open(conf.c_str());
		if (config.fail())
		{
			cerr << "Unable to find or read configuration file [--conf] [-C] default : paraload.conf" << conf << endl;
			return(1);
		}
		port_number = stoi(port);
		if (port_number > 65535)
		{
			cerr << "Port number must be smaller than 65535" << endl;
			return(1);
		}
		if (port_number < 10000)
		{
			cerr << "Port number must be greater than 10000" << endl;
			return(1);
		}
		
	}
	if (client == "yes")
	{
		if (port == "")
		{
			cerr << "Please specify	a port [--port] [-p]" << endl;
			return(1);
		}
	}
	if (tool == "check")
	{
		if (input == "")
		{
			cerr << "Please specify an input [--input] [-i]" << endl;
			return(1);
		}
		in.open(input.c_str());
		if (in.fail())
		{
			cerr << "Unable to find or read input file [--input] [-i]" << endl;
			return(1);
		}
		if (log == "")
		{
			cerr << "Please specify a log [--log] [-l]" << endl;
			return(1);
		}
		lo.open(log.c_str());
		if (lo.fail())
		{
			cerr << "Unable to find or read log file [--log] [-l]" << endl;
			return(1);
		}
		config.open(conf.c_str());
		if (config.fail())
		{
			cerr << "Unable to find or read configuration file [--conf] [-C] default : paraload.conf" << conf << endl;
			return(1);
		}
	}
	if (tool == "time")
	{
		if (log == "")
		{
			cerr << "Please specify a log [--log] [-l]" << endl;
			return(1);
		}
		lo.open(log.c_str());
		if (lo.fail())
		{
			cerr << "Unable to find or read log file [--log] [-l]" << endl;
			return(1);
		}
	}
	if (tool == "reorder")
	{
		if (log == "")
		{
			cerr << "Please specify a log [--log] [-l]" << endl;
			return(1);
		}
		lo.open(log.c_str());
		if (lo.fail())
		{
			cerr << "Unable to find or read log file [--log] [-l]" << endl;
			return(1);
		}
		if (output ==  "")
		{
			cerr << "Please specify an output [--out] [-o]" << endl;
		}
		ou.open(output.c_str());
		if (ou.fail())
		{
			cerr << "Unable to find or read output file [--output] [-o]" << endl;
			return(1);
		}
	}
	if (tool == "clients")
	{
		if (log == "")
		{
			cerr << "Please specify a log [--log] [-l]" << endl;
			return(1);
		}
		lo.open(log.c_str());
		if (lo.fail())
		{
			cerr << "Unable to find or read log file [--log] [-l]" << endl;
			return(1);
		}
	}
	return(0);
}


void Cline::help_func()
{
	cout << "Run a server: line between {} are optional" << endl;
	cout << "paraload --server(-s) --port(-p) [10000-65535] --input(-i) [input_file] --output(-o) [output_file] --log(-l) [log_file] { --bg(-b) (background) --report(-r) [report_file] } {--conf(-C) [conf_file default:paraload.conf] }" << endl << endl;
	cout << "Run a client:" << endl;
	cout << "paraload --client(-c) --port(-p) [ same as the server ] --host(-h) [ host of the server default:localhost ] --fails(-f) [ max number of command fail allowed default:-1 (infinity)]" << endl << endl;
	cout << "Check if the server is responding:" << endl;
	cout << "paraload --client(-c) --port(-p) [ same as the server ] --ping(-P)" << endl << endl;
	cout << "Get informations about running clients:" << endl;
	cout << "paraload --client(-c) --port(-p) [ same as the server ] --info(-I)" << endl << endl;
	cout << "Check integrity after a run:" << endl;
	cout << "paraload --tool(-t) check --input(-i) [input_file] --log(-l) [log_file] {--conf(-C) [conf_file default:paraload.conf] }" << endl << endl;
	cout << "Reorder output with the same order as the input:" << endl;
	cout << "paraload --tool(-t) reorder --output(-o) [output_file] --outsort(-O) [output_file sorted]" << endl << endl;
	cout << "Get cumulated time of run (even if the run is not completed):" << endl;
	cout << "paraload --tool(-t) time --log(-l) [log_file]" << endl << endl;
	cout << "Get informations about terminated computations on clients (even if the run is not completed):" << endl;
	cout << "paraload --tool(-t) clients --log(-l) [log_file]" << endl << endl;
	cout << "Print this:" << endl;
	cout << "paraload --help(-?)" << endl << endl;
}



void Cline::version_func()
{
	cout << "\nVersion : " << VERSION << endl;
	cout << "Compiled by " << USER_LOGIN << endl;
	cout << "with " << GCC_VERSION << endl << endl;
}


