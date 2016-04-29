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
	std::string tool;
	std::string outsort;
	public:
	Cline(int argc,char **argv);
	int check();
	const std::string& getcmd(std::string what);
	void help_func();
	void version_func();
};

#endif
