/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "error.h"

int errors;
int warnings;
int dbflag = 1;

void fatal(char *format, ...)
{
        va_list list;
        va_start(list, format);
        vprintf(format, list);
        va_end(list);
}

void error(char *format, ...)
{
        va_list list;
        va_start(list, format);
        vprintf(format, list);
        va_end(list);
        errors++;
}

void warning(char *format, ...)
{
        va_list list;
        va_start(list, format);
        vprintf(format, list);
        va_end(list);
        warnings++;
}

void debug(char *format, ...)
{
        va_list list;
        va_start(list, format);
        vprintf(format, list);
        va_end(list);
}
