#include "Cline.hpp"
#include <getopt.h>
#include <iostream>
#include <fstream>

using namespace::std;

Cline::Cline(int argc,char **argv)
{
	int c;
	int option_index = 0;
 	struct option opt[19] =
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


	while( (c = getopt_long(argc,argv,"i:o:C:h:p:l:S:scwv?VPIbr:f:",opt,&option_index)) != -1)
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
		}
	}
}


string Cline::getcmd(string what)
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
	else
	return "";
}

int Cline::check()
{
	int port_number;
	
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
		return(1);
	}
	if ((client == "no") && (server == "no"))
	{
		cerr << "Make a choice: run a server with [--server] [-s] or a client with [--client] [-c]" << endl;
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
		ifstream config(conf.c_str());
		if (!config)
		{
			cerr << "Unable to find or read configuration file " << conf << endl;
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
	return(0);
}


void Cline::help_func()
{
	cout << "Run a server: line between {} are optional" << endl;
	cout << "./paraload --server --port [10000-65535] --input [input_file] --output [output_file] --log [log_file] { --bg(background) -r [report_file] } {--conf [conf_file default:paraload.conf] }" << endl << endl;
	cout << "Run a client:" << endl;
	cout << "./paraload --client --port [ same as the server ] --host [ host of the server default:localhost ] --fails [ max number of command fail allowed default:-1 (infinity)]" << endl << endl;
}



void Cline::version_func()
{
	cout << "\nSVN revision: " << SVN_REV << endl;
	cout << "Compiled by " << USER_LOGIN << endl;
	cout << "with " << GCC_VERSION << endl << endl;
}


