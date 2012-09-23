#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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
    return 0;
}

void dataLogV(const char* format, va_list)
{
}

void dataLog(const char* format, ...)
{
}

void dataLogString(const char*)
{
}

}
