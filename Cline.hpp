#ifndef CLINE_H
#define CLINE_H


#include <string>


class Cline
{
	private:
	std::string input;
	std::string output;
	std::string conf;
	std::string log;
	std::string host;
	std::string port;
	std::string server;
	std::string client;
	std::string watch;
	std::string verbose;
	std::string shm;
	std::string help;
	std::string version;
	std::string ping;
	std::string info;
	std::string bg;
	std::string report;
	std::string fails;
	std::string voidstring;
	public:
	Cline(int argc,char **argv);
	int check();
	const std::string& getcmd(std::string what);
	void help_func();
	void version_func();
};

#endif
