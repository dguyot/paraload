#ifndef INDEX_H
#define INDEX_H

#include <vector>
#include <string>

class Index
{
	private:
	std::vector<std::streampos> index;
	std::vector <std::streampos>::size_type size;
	std::string filename;
	std::string sentinel;
	public:
	Index(std::string filename, std::string sentinel);
	std::streampos getpos(std::vector <std::streampos>::size_type which);
	std::vector<std::streampos>::size_type getsize();
	std::string get_filename();
	std::string get_sentinel();
};

#endif
