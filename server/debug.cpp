#include "debug.h"
char* log_type_str[]={"INFO","WARN","BUG"};
void log(log_type lt, const char *format, ...)
{
#ifdef DEBUG
    if(lt < DEBUG_LEVEL)
        return;
    char buffer[256];
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    strftime (buffer, 256, "%d/%b/%Y %X", now);
    printf("%s: %s ",buffer,log_type_str[lt]);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif /* DEBUG */
}
