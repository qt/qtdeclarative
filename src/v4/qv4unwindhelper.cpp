#include <qv4unwindhelper.h>

#include <wtf/Platform.h>

#if CPU(X86_64) && (OS(LINUX) || OS(MAC_OS_X))
#  define USE_DW2_HELPER
#elif CPU(X86) && OS(LINUX)
#  define USE_DW2_HELPER
#elif OS(WINDOWS)
    // SJLJ will unwind on Windows
#  define USE_NULL_HELPER
#elif OS(IOS)
    // SJLJ will unwind on iOS
#  define USE_NULL_HELPER
#elif OS(ANDROID)
#  warning "TODO!"
#  define USE_NULL_HELPER
#else
#  warning "Unsupported/untested platform!"
#  define USE_NULL_HELPER
#endif

#ifdef USE_DW2_HELPER
#  include <qv4unwindhelper_p-dw2.h>
#endif // USE_DW2_HELPER

#ifdef USE_NULL_HELPER
using namespace QQmlJS::VM;
void UnwindHelper::registerFunction(Function *function) {Q_UNUSED(function);}
void UnwindHelper::registerFunctions(QVector<Function *> functions) {Q_UNUSED(functions);}
void UnwindHelper::deregisterFunction(Function *function) {Q_UNUSED(function);}
void UnwindHelper::deregisterFunctions(QVector<Function *> functions) {Q_UNUSED(functions);}
#endif // USE_NULL_HELPER

