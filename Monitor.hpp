#ifndef MONITOR_H
#define MONITOR_H
#include <vector>
#include <utility>
#include <unordered_map>
#include <fstream>
#include "Index.hpp"
#include "Cline.hpp"
#include "netinet/in.h"

class Monitor
{
	private:
	std::vector <int> states;
	std::vector <std::streampos>::size_type size;
	std::unordered_map <long, std::pair < std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> > current_addr_idx;
	long todo;
	long inprogress;
	long done;
	long fail;
	std::string log_file;
	public:
	Monitor(Index*,Cline*);
	int restart();
	int tag_segment(int state, std::vector<std::streampos>::size_type begin, std::vector<std::streampos>::size_type end);
	int atomic_state(std::vector<std::streampos>::size_type which);
	int add_addr_idx(int ip, int port, std::vector<std::streampos>::size_type begin, std::vector<std::streampos>::size_type end);
	std::vector<std::streampos>::size_type rem_addr_idx(int ip, int port);
	std::vector<std::streampos>::size_type exists_addr_idx(int ip, int port);
	long count_todo();
	long count_inprogress();
	long count_done();
	long count_fail();
	int chgstate(int state1, int state2);
};
#endif
