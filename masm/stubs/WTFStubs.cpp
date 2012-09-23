#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <qdebug.h>

namespace WTF {

void* fastMalloc(unsigned int size)
{
    return malloc(size);
}

void fastFree(void* ptr)
{
    free(ptr);
}

int cryptographicallyRandomNumber()
{
    return 0;
}

FILE* dataFile()
{
    return stdout;
}

void dataLogV(const char* format, va_list args)
{
    qDebug(format, args);
}

void dataLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    qDebug(format, args);
    va_end(args);
}

void dataLogString(const char* str)
{
    qDebug("%s", str);
}

}
