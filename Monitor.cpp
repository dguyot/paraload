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
	
	todo = (long)size;
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

int Monitor::add_addr_idx(int ip, int port, std::vector<std::streampos>::size_type begin, std::vector<std::streampos>::size_type end)
{
	long key = ((long)ip << 32) + (long)port;
	pair <std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> segment (begin,end);
	current_addr_idx[key] = segment;
	tag_segment(INPROGRESS, segment.first, segment.second);
	return(0);
}

std::vector<std::streampos>::size_type Monitor::rem_addr_idx(int ip, int port)
{
	long key = ((long)ip << 32) + (long)port;
	pair <std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> segment;
	segment = current_addr_idx[key];
	current_addr_idx.erase(key);
	//cerr << "REM " << segment.first << ":" << segment.second << endl;
	tag_segment(DONE, segment.first, segment.second);
	return(segment.first);
}

std::vector<std::streampos>::size_type Monitor::exists_addr_idx(int ip, int port)
{
	std::unordered_map <long, std::pair < std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type>>::const_iterator got;
	pair <std::vector<std::streampos>::size_type, std::vector<std::streampos>::size_type> segment;
	long key = ((long)ip << 32) + (long)port;
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


long Monitor::count_todo()
{
	return(todo);
}

long Monitor::count_inprogress()
{
	return(inprogress);
}

long Monitor::count_done()
{
	return(done);
}

long Monitor::count_fail()
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
