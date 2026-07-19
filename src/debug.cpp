#ifdef _WIN32
#include "windows.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#include "filehandling.h"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

FILE *fp = 0;

void output_debug_message(const char *fmt, ...)
{
    char text[1024];
    va_list ap;

    if (fmt == 0)
        return ;

    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    if (fp == 0)
		fp = f1open("roadfighter.dbg", "w", USERDATA);

    if (fp == 0) return; /* couldn't open the log (e.g. non-writable folder) -
                             debug logging must never be able to crash the game */

    fprintf(fp, "%s", text);
    fflush(fp);
} /* glprintf */




void close_debug_messages(void)
{
    fclose(fp);
    fp = 0;
} /* close_debug_messages */
