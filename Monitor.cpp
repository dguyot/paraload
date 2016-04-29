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


#include "Monitor.hpp"
#include "Define.hpp"
#include <iostream>
#include <sstream>

using namespace std;

Monitor::Monitor(Index* index,Cline* cline)
{
	vector<streampos>::size_type pos;

	log_file = cline->getcmd("log");
	size = index->getsize();
	cerr << "Making monitoring structure ..." << endl;
	for (pos = 0; pos <= size; pos++)
	{
		states.push_back(TODO);
	}
	
	todo = (uint64_t)size;
	inprogress = 0;
	done = 0;
	fail = 0;
	states.shrink_to_fit();
	cerr << "Monitoring structure done." << endl << endl;
	restart();
}

int Monitor::tag_segment(int state, vector<streampos>::size_type begin,vector<streampos>::size_type end)
{
	vector<streampos>::size_type pos;
	
	if (begin > size) return(-1);
	if (end > size) return(-1);
	
	for (pos = begin; pos < end; pos++)
	{
		if (states[pos] == TODO) todo--;
		else if (states[pos] == INPROGRESS) inprogress--;
		else if (states[pos] == DONE) done--;
		else if (states[pos] == FAIL) fail--;
		
		states[pos] = state;
		
		if (states[pos] == TODO) todo++;
		else if (states[pos] == INPROGRESS) inprogress++;
		else if (states[pos] == DONE) done++;
		else if (states[pos] == FAIL) fail++;
		
	}
	
	return(0);
}

int Monitor::atomic_state(std::vector<std::streampos>::size_type which)
{
	if (which > size) return(-1);
	else
	return states[which];
}

int Monitor::add_addr_idx(uint32_t ip, uint32_t port, std::vector<std::streampos>::size_type begin, std::vector<std::streampos>::size_type end)
{
	uint64_t key = ((uint64_t)ip << 32) + (uint64_t)port;
	pair <std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> segment (begin,end);
	current_addr_idx[key] = segment;
	tag_segment(INPROGRESS, segment.first, segment.second);
	return(0);
}

std::vector<std::streampos>::size_type Monitor::rem_addr_idx(uint32_t ip, uint32_t port)
{
	uint64_t key = ((uint64_t)ip << 32) + (uint64_t)port;
	pair <std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> segment;
	segment = current_addr_idx[key];
	current_addr_idx.erase(key);
	//cerr << "REM " << segment.first << ":" << segment.second << endl;
	tag_segment(DONE, segment.first, segment.second);
	return(segment.first);
}

std::vector<std::streampos>::size_type Monitor::exists_addr_idx(uint32_t ip, uint32_t port)
{
	std::unordered_map <uint64_t, std::pair < std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type>>::const_iterator got;
	pair <std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> segment;
	uint64_t key = ((uint64_t)ip << 32) + (uint64_t)port;
	got = current_addr_idx.find(key);
	if (got == current_addr_idx.end())
	{
		return(-1);
	}
	else
	{
		segment = got->second;
		current_addr_idx.erase(key);
		tag_segment(TODO, segment.first, segment.second);
		return(segment.first);
	}
}


int Monitor::restart()
{
	int bad_return = 0;
	string line;
	string field;
	vector<streampos>::size_type index_b;
	vector<streampos>::size_type index_e;
	int rt_value;
	ifstream log(log_file.c_str());
	log.seekg(0,std::ios::beg);
	cerr << "Reading log file ..." << endl;
	while(getline(log,line))
	{
		istringstream iss(line);
		iss >> index_b;
		iss >> index_e;
		iss >> field;
		iss >> field;
		iss >> field;
		iss >> field;
		iss >> rt_value;
		if (rt_value != 0)
		{
			bad_return++;
			tag_segment(TODO, index_b, index_e);
		}
		else
		{
			tag_segment(DONE, index_b, index_e);
		}
	}
	cerr << "Bad return: " << bad_return << endl;
	cerr << "Already good: " << count_done() << endl;
	cerr << "To compute: " << count_todo() << endl << endl;
	return(0);
}


uint64_t Monitor::count_todo()
{
	return(todo);
}

uint64_t Monitor::count_inprogress()
{
	return(inprogress);
}

uint64_t Monitor::count_done()
{
	return(done);
}

uint64_t Monitor::count_fail()
{
	return(fail);
}


int Monitor::chgstate(int state1, int state2)
{
	vector<streampos>::size_type pos;
	vector<streampos>::size_type end;
	
	end = states.size();
	
	for(pos = 0; pos < end; pos++)
	{	
		if (states[pos] == state1)
		{
			if (state1 == FAIL) fail--;
			else if (state1 == TODO) todo--;
			else if (state1 == INPROGRESS) inprogress--;
			else if (state1 == DONE) done--;
			states[pos] = state2;
			if (state2 == TODO) todo++;
			else if (state2 == INPROGRESS) inprogress++;
			else if (state2 == DONE) done++;
			else if (state2 == FAIL) fail++;
		}
	}
	
	return(0);
}
