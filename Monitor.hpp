#ifndef MONITOR_H
#define MONITOR_H

#define _FILE_OFFSET_BITS 64 //to use very large files even on 32 bits systems
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
	uint64_t todo;
	uint64_t inprogress;
	uint64_t done;
	uint64_t fail;
	std::vector <int> states;
	std::vector <std::streampos>::size_type size;
	std::unordered_map <uint64_t, std::pair < std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> > current_addr_idx;
	std::string log_file;
	public:
	Monitor(Index*,Cline*);
	uint64_t count_todo();
	uint64_t count_inprogress();
	uint64_t count_done();
	uint64_t count_fail();
	std::vector<std::streampos>::size_type rem_addr_idx(uint32_t ip, uint32_t port);
	std::vector<std::streampos>::size_type exists_addr_idx(uint32_t ip, uint32_t port);
	int restart();
	int tag_segment(int state, std::vector<std::streampos>::size_type begin, std::vector<std::streampos>::size_type end);
	int atomic_state(std::vector<std::streampos>::size_type which);
	int add_addr_idx(uint32_t ip, uint32_t port, std::vector<std::streampos>::size_type begin, std::vector<std::streampos>::size_type end);
	int chgstate(int state1, int state2);
	long padding;//to align class on every cpus
};
#endif
