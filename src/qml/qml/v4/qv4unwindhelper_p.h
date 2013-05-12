#ifndef QV4UNWINDHELPER_H
#define QV4UNWINDHELPER_H

#include <QtCore/QVector>

namespace QV4 {

struct Function;

class UnwindHelper
{
public:
    static void ensureUnwindInfo(Function *function);
    static void registerFunction(Function *function);
    static void registerFunctions(const QVector<Function *> &functions);
    static void deregisterFunction(Function *function);
    static void deregisterFunctions(const QVector<Function *> &functions);
#ifdef Q_PROCESSOR_ARM
    static int unwindInfoSize();
    static void writeARMUnwindInfo(void *codeAddr, int codeSize);
#endif
};

}

#endif // QV4UNWINDHELPER_H
