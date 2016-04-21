#include "Fetch.hpp"
#include "Define.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>

using namespace::std;

Fetch::Fetch(string policy, string policy_limit, Index *index, Monitor *monitor)
{
	buffer = 0;
	this->index = index;
	this->monitor = monitor;
	size_index = index->getsize();
	file_size = index->getpos(index->getsize());
	end_index = 0;
	begin_index = 0;
	if (policy.find("STATIC") == 0)
	this->policy = STATIC;
	else if (policy.find("LINEAR") == 0)
	this->policy = LINEAR;
	else if (policy.find("SQUARE") == 0)
	this->policy = SQUARE;
	else if (policy.find("CUBIC") == 0)
	this->policy = CUBIC;
	else if (policy.find("NLOGN") == 0)
	this->policy = NLOGN;
	this->policy_limit = (streampos)strtoul(policy_limit.c_str(),NULL,10);
}

Fetch::~Fetch()
{
	if (buffer != 0) delete[] buffer;
}

streampos Fetch::make_chunk()
{
	begin_index = end_index;
	
	streampos limit = 0;
	streampos reciprocity_limit = 0;
	streampos atomic_size = 0;
	streampos chunk_size = 0;

	ifstream input_file(((index)->get_filename()).c_str());
	
	//search for the first segment to do
	while ((monitor->atomic_state(end_index) != TODO) && (end_index < size_index))
	{
		begin_index++;
		end_index++;
	}
	
	input_file.seekg(index->getpos(begin_index));

	switch (policy)
	{
		case STATIC:
		{
			while ((monitor->atomic_state(end_index) == TODO) && (limit < policy_limit) && (end_index < size_index))
			{
				end_index++;
				limit += 1;
				//cerr << end_index << "\t" << limit << endl;
			}
			chunk_size = (index->getpos(end_index) - index->getpos(begin_index));
			break;	
		}		
		case LINEAR:
		{
			while ((monitor->atomic_state(end_index) == TODO) && (limit < policy_limit) && (end_index < size_index))
			{
				end_index++;
				atomic_size = (index->getpos(end_index) - index->getpos(end_index-1));
 				limit += atomic_size;
				chunk_size += atomic_size;
			}
			break;
		}
		case SQUARE:
		{
			while ((monitor->atomic_state(end_index) == TODO) && (reciprocity_limit < policy_limit) && (end_index < size_index))
			{
				end_index++;
				atomic_size = (index->getpos(end_index) - index->getpos(end_index-1));
 				limit += atomic_size * atomic_size;
 				reciprocity_limit = (streampos)(sqrt((double)limit));
				chunk_size += atomic_size;
			}
			break;
		}
		case CUBIC:
		{
			while ((monitor->atomic_state(end_index) == TODO) && (reciprocity_limit < policy_limit) && (end_index < size_index))
			{
				end_index++;
				atomic_size = (index->getpos(end_index) - index->getpos(end_index-1));
 				limit += atomic_size * atomic_size * atomic_size;
 				reciprocity_limit = (streampos)(cbrt((double)limit));
				chunk_size += atomic_size;
			}
			break;
		}
		case NLOGN:
		{
			while ((monitor->atomic_state(end_index) == TODO) && (reciprocity_limit < policy_limit) && (end_index < size_index))
			{
				end_index++;
				atomic_size = (index->getpos(end_index) - index->getpos(end_index-1));
 				limit += atomic_size * (streampos)(log((double)atomic_size));
 				reciprocity_limit = (streampos) (((double)limit)/log(((double)limit)/log((double)limit)));
				chunk_size += atomic_size;
			}
			break;
		}
	}
	
	if (buffer != 0) delete[] buffer;
	if (chunk_size == 0)
	{
		buffer = 0;
		return(chunk_size);
	}
	buffer = new char[chunk_size + (streampos)1];
	input_file.read(buffer,chunk_size);
	buffer[chunk_size] = '\0';
	return chunk_size;
}

char* Fetch::Get_chunk()
{
	return buffer;
}

vector<streampos>::size_type Fetch::Index_begin()
{
	return begin_index;
}

vector<streampos>::size_type Fetch::Index_end()
{
	return end_index;
}

std::streampos Fetch::Offset_begin()
{
	return index->getpos(begin_index);
}

std::streampos Fetch::Offset_end()
{
	return index->getpos(end_index);	
}

int Fetch::rewind(std::vector<std::streampos>::size_type offset)
{
	if (offset < begin_index)
	{
		cerr << "Rewind to index \t" << offset << endl;
		begin_index = offset;
		end_index = offset;
		return(0);
	}
	if (offset == (std::vector<std::streampos>::size_type)(-1))
	{
		cerr << "Rewind to index \t" << offset << endl;
		begin_index = offset;
		end_index = 0;
		return(0);
	}
	return(1);
}
