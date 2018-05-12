#ifndef _DEBUG_H_
#define _DEBUG_H_
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#define DEBUG
#define DEBUG_LEVEL INFO

enum log_type {VERBOSE,INFO,WARN,BUG};
void log(log_type lt, const char *format, ...);
void prepara_segnali();
void disattiva_segnali();
void attiva_segnali();
void  print_trace (void);
#endif
