#ifndef FETCH_H
#define FETCH_H

#define _FILE_OFFSET_BITS 64 //to use very large files even on 32 bits systems

#include <vector>
#include <string>
#include "Index.hpp"
#include "Monitor.hpp"

class Fetch
{
	private:
	Index *index;
	Monitor *monitor;
	std::streampos file_size;
	std::streampos policy_limit;
	std::vector <std::streampos>::size_type end_index;
	std::vector <std::streampos>::size_type begin_index;
	std::vector <std::streampos>::size_type size_index;
	char* buffer;
	int policy;
	int padding;//to align class on 64 bits
	public:
	Fetch(std::string policy, std::string policy_limit, Index *index, Monitor *monitor);
	~Fetch();
	std::streampos make_chunk();
	std::vector<std::streampos>::size_type Index_end();
	std::vector<std::streampos>::size_type Index_begin();
	std::streampos Offset_begin();
	std::streampos Offset_end();
	char* Get_chunk();
	int rewind(std::vector<std::streampos>::size_type offset);
};

#endif
