#include "debug.h"
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <execinfo.h>

char* log_type_str[]= {"VERBOSE","INFO","WARN","BUG"};
void log(log_type lt, const char *format, ...)
{
#ifdef DEBUG
	if(lt < DEBUG_LEVEL)
		return;
	char buffer[256];
	time_t t = time(0);   // get time now
	struct tm * now = localtime( & t );
	//strftime (buffer, 256, "%d/%b/%Y %X", now);
	strftime (buffer, 256, "%X", now);
	printf("%s: (%d) %s ",buffer,getpid(),log_type_str[lt]);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
#endif /* DEBUG */
}


static void sighandler(int sig)
{

	std::cout << "segfault"<<std::endl;
	print_trace();
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, sig);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	throw std::string ("corretto");
}


void prepara_segnali()
{
	
	return;
	/*
	signal(SIGSEGV, sighandler);
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGSEGV);
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);*/
}

void attiva_segnali()
{
	
	signal(SIGSEGV, sighandler);
	signal(SIGABRT, sighandler);
	signal(SIGALRM, sighandler);
	alarm(3);
	return;
	/*sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGSEGV);
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);*/
}

void disattiva_segnali()
{
	signal(SIGALRM, SIG_IGN);
	alarm(0);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	return;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGSEGV);
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}


void  print_trace (void)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	printf ("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf ("%s\n", strings[i]);

	free (strings);
}
