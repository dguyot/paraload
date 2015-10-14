#include "Index.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <istream>

using namespace::std;

Index::Index(string filename, string sentinel)
{
	streampos fsize;
	this->filename = filename;
	this->sentinel = sentinel;
	cerr << "Indexing " << filename << " with lines beginning by " << "\"" << sentinel << "\"" << " ..." << endl;
	size = 0;
	string line;
	streampos pos = 0;
	ifstream input_file(filename.c_str());
	input_file.seekg(0,std::ios::end);
	fsize = input_file.tellg();
	input_file.seekg(0,std::ios::beg);
	while(getline(input_file,line))
	{
		if (line.find(sentinel) == 0)
		{
			pos = input_file.tellg() - (streampos)(line.length()) - 1;
			index.push_back(pos);
			//index.emplace_back(pos);
			size++;
		}
	}
	index.push_back(fsize);
	index.shrink_to_fit();
	cerr << "Index done : " << size << " entry points." << endl << endl;
}


streampos Index::getpos(vector<streampos>::size_type which)
{
	//if (which < this->index.size)
	return index[which];
	//else
	//return -1;
}

vector<streampos>::size_type Index::getsize()
{
	return size;
}

string Index::get_filename()
{
	return filename;
}


string Index::get_sentinel()
{
	return sentinel;
}

