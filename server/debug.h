#ifndef _DEBUG_H_
#define _DEBUG_H_
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#define DEBUG

enum log_type {INFO,BUG};

void log(log_type, const char *format, ...)
{
#ifdef DEBUG
    char buffer[256];
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    strftime (buffer, 256, "%d/%b/%Y %X: ", now);
    printf("%s",buffer);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif /* DEBUG */
}

#endif
