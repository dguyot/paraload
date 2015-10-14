#include "Cline.hpp"
#include "Conf.hpp"
#include "Index.hpp"
#include "Fetch.hpp"
#include "Comm_S.hpp"
#include "Comm_C.hpp"
#include "Monitor.hpp"
#include <iostream>
#include <fstream>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <ctime>

sigjmp_buf stack_client_save;

using namespace::std;

ofstream report;

static int belzebuth(Cline* cline)
{
	
	char hostname[256];
	pid_t pid = fork();//fork and kill your father
	report.open(cline->getcmd("report"));//redirect cerr to report
	cerr.rdbuf(report.rdbuf());
	gethostname(hostname,256);
	if (pid < 0)
	{
		perror("fork failed : can't run in background");
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
		exit(0);
	}
	pid = getpid();
	cerr << "HOST: " << hostname << "::" << cline->getcmd("port") << endl;
	cerr << "PID: " << pid << endl << endl;
	cerr << "INPUT: " << cline->getcmd("input") << endl;
	cerr << "OUTPUT: " << cline->getcmd("output") << endl;
	cerr << "LOG: " << cline->getcmd("log") << endl << endl;;
	
	return(0);
}


static int server(Cline* cline)
{
	streampos chunk_size;
	streampos offout_b;
	streampos offout_e;
	std::vector<std::streampos>::size_type current_index;
	long current_round;
	long nb_round;
	int action;
	time_t timer;
	time_t begin_time;
	
	time(&begin_time);//get the time of beginning

	signal(SIGPIPE, SIG_IGN); //if the connection is lost during a write ignore SIGPIPE
	
	cerr << "Starting paraload server ..." << endl << endl;
	
	cerr << "Input: " << cline->getcmd("input") << endl;
	cerr << "Output: " << cline->getcmd("output") << endl;
	cerr << "Log file: " << cline->getcmd("log") << endl << endl;

	Conf *conf = new Conf(cline->getcmd("conf"),'=');
	
	Index *index = new Index(cline->getcmd("input"), conf->getconf("sentinel"));

	Monitor *monitor = new Monitor(index, cline);

	Fetch *fetch = new Fetch(conf->getconf("policy"), conf->getconf("policy_limit"), index, monitor);

	Comm_S *comm_s = new Comm_S(conf, cline);
	
	nb_round = atol(conf->getconf("round").c_str());
	current_round = 0;
	
	ofstream out((cline->getcmd("output")).c_str(), ofstream::app);
	ofstream log((cline->getcmd("log")).c_str(), ofstream::app);
	
	if (cline->getcmd("bg") == "yes") belzebuth(cline);
	cerr << "Starting round " << current_round << " / " << nb_round << endl;

	while ((current_round < nb_round) && ((monitor->count_todo() != 0) || (monitor->count_inprogress() != 0)))
	{
		if (monitor->count_todo() == 0)
			comm_s->set_signal_flag(PLD_SIG_STOP);
		else
		{
			comm_s->set_signal_flag(PLD_SIG_OK);
		
			if (fetch->Index_end() >= index->getsize())
			{
				cerr << "Rewinding list of jobs" << endl;
				fetch->rewind(0);
				current_round++;
				cerr << "Starting round " << current_round << " / " << nb_round << endl;
			}
		}
		action = comm_s->wait();

		switch (action)
		{
			case PLD_GET:
			{
				fetch->make_chunk();
				
				if (comm_s->send(fetch->Index_begin(), fetch->Index_end(), fetch->Offset_end()-fetch->Offset_begin(), fetch->Get_chunk()) == PLD_OK)
				{
					//monitor->tag_segment(INPROGRESS, fetch->Index_begin(), fetch->Index_end());
					monitor->add_addr_idx(comm_s->get_ip_int(),comm_s->get_port(), fetch->Index_begin(), fetch->Index_end());
				}
				else fetch->rewind(fetch->Index_begin());
				break;
			}
			case PLD_PUT:
			{
				if (comm_s->receive() == PLD_OK)
				{
					offout_b = out.tellp();
					out << comm_s->getres();
					out.flush();
					offout_e = out.tellp();
					log << comm_s->get_idx_begin() << "\t" << comm_s->get_idx_end() << "\t";
					log << index->getpos(comm_s->get_idx_begin()) << "\t" << index->getpos(comm_s->get_idx_end()) << "\t";
					log << offout_b << "\t" << offout_e << "\t";
					log << comm_s->get_rt_value() << "\t";
					log << comm_s->get_ip() << "\t";
					log << comm_s->get_rtime() << "\t" << comm_s->get_utime() << "\t" << comm_s->get_ktime() << endl;
					log.flush();
					
					if (comm_s->get_rt_value() == 0)
					{
						monitor->rem_addr_idx(comm_s->get_ip_int(),comm_s->get_port());
						//monitor->tag_segment(DONE, comm_s->get_idx_begin(), comm_s->get_idx_end());
					}
					else
					{
						time(&timer);
						monitor->tag_segment(TODO, comm_s->get_idx_begin(), comm_s->get_idx_end());
						cerr << "!!\t" << comm_s->get_ip() << "::" << comm_s->get_port() << "\t[" << comm_s->get_idx_begin() << "," << comm_s->get_idx_end() << "]\t" << comm_s->get_rt_value() << "\t" << timer - begin_time << "\t" << asctime(localtime(&timer));
					}

				}
				break;
			}
			case PLD_CON:
			{
				comm_s->add_client();
				time(&timer);
				cerr << "++\t" << comm_s->get_ip() << "::" << comm_s->get_port() << "\t" << timer - begin_time << "\t" << asctime(localtime(&timer));
				break;
			}
			case PLD_DEC:
			{
				current_index = monitor->exists_addr_idx(comm_s->get_ip_int(),comm_s->get_port());
				time(&timer);
				cerr << "--\t" << comm_s->get_ip() << "::" << comm_s->get_port() << "\t" << timer - begin_time << "\t" << asctime(localtime(&timer));
				if (current_index != (std::vector<std::streampos>::size_type)(-1)) fetch->rewind(current_index);
				comm_s->remove_client();

				break;
			}
			case PLD_CONT:
			{
				break;
			}
			case PLD_PING:
			{
				comm_s->pong();
				break;
			}
			case PLD_INFO:
			{
				comm_s->info(monitor->count_todo(), monitor->count_inprogress(), monitor->count_done());
				break;
			}
		}
	}
	
	comm_s->set_signal_flag(PLD_SIG_STOP);
	cerr << "End of run, killing all remaining clients" << endl;
	while (comm_s->get_nb_clients())
	{
		action = comm_s->wait();
		switch (action)
		{
			case PLD_DEC:
			{
				monitor->exists_addr_idx(comm_s->get_ip_int(),comm_s->get_port());
				time(&timer);
				cerr << "--\t" << comm_s->get_ip() << "::" << comm_s->get_port() << "\t" << timer - begin_time << "\t" << asctime(localtime(&timer));
				comm_s->remove_client();
				break;
			}
			case PLD_CON:
			{
				comm_s->add_client();
				time(&timer);
				cerr << "++\t" << comm_s->get_ip() << "::" << comm_s->get_port() << "\t" << timer - begin_time << "\t" << asctime(localtime(&timer));
				break;
			}
			case PLD_PING:
			{
				comm_s->pong();
				break;
			}
			case PLD_INFO:
			{
				comm_s->info(monitor->count_todo(), monitor->count_inprogress(), monitor->count_done());
				break;
			}
		}
	}
	
	cerr << "Run terminated : " << monitor->count_todo() << " remaining jobs" << endl;
	delete conf;
	delete index;
	delete monitor;
	delete fetch;
	delete comm_s;
	return(EXIT_SUCCESS);
}

static void client_signal_handler(int n)
{
	cerr << "Client killed by signal " << n << "(" << sys_siglist[n] << ")" << endl;
	cerr << "Sending " << sys_siglist[n] <<" to to all the lineage...." << endl;
	cerr.flush();
	signal(n,SIG_IGN);//ignore current signal 
	killpg(getpgrp(), n);//transmit the signal to all the lineage
	siglongjmp(stack_client_save,1);
}


static int client(Cline* cline)
{
	int rt;
	int max_allowed_fails = stoi(cline->getcmd("fails"));
	
	Comm_C *comm_c = new Comm_C(cline);
	
	signal(SIGTERM, client_signal_handler);
	signal(SIGINT, client_signal_handler);
	signal(SIGPIPE, client_signal_handler);
	signal(SIGQUIT, client_signal_handler);

	
	if (sigsetjmp(stack_client_save,1) == 0)
	{
		if (cline->getcmd("ping") == "yes")
		{
			comm_c->ping();
			return(EXIT_SUCCESS);
		}
	
		if (cline->getcmd("info") == "yes")
		{
			comm_c->info();
			return(EXIT_SUCCESS);
		}
		if (max_allowed_fails == -1)
		{
			while(1)
			{
				rt = comm_c->receive();
				if (rt != PLD_OK) break;
				rt = comm_c->run();
				if (rt != PLD_OK) break;
				comm_c->send();
			}
		}
		else
		{
			while(max_allowed_fails > comm_c->fails())
			{
				rt = comm_c->receive();
				if (rt != PLD_OK) break;
				rt = comm_c->run();
				if (rt != PLD_OK) break;
				comm_c->send();
			}
		}
	}
	delete comm_c;
	return(EXIT_SUCCESS);
}


int main(int argc,char** argv)
{
	int rt;
	Cline *cline = new Cline(argc,argv);
	if (cline->check() != 0) return(EXIT_FAILURE);
	
	if (cline->getcmd("server") == "yes")
	{
		rt = server(cline);
		delete cline;
		return(rt);
	}
	
	if (cline->getcmd("client") == "yes")
	{
		rt = client(cline);
		delete cline;
		return(rt);
	}
	return(EXIT_FAILURE);
}


