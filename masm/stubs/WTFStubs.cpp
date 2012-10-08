#include <config.h>
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

uint32_t cryptographicallyRandomNumber()
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

extern "C" {

void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion)
{
}

void WTFReportBacktrace()
{
}

void WTFInvokeCrashHook()
{
}

}


#if ENABLE(ASSEMBLER) && CPU(X86) && !OS(MAC_OS_X)
#include <MacroAssemblerX86Common.h>

JSC::MacroAssemblerX86Common::SSE2CheckState JSC::MacroAssemblerX86Common::s_sse2CheckState = JSC::MacroAssemblerX86Common::NotCheckedSSE2;
#endif

