#ifndef QV4UNWINDHELPER_H
#define QV4UNWINDHELPER_H

#include <QtCore/QVector>

namespace QQmlJS {
namespace VM {

struct Function;

class UnwindHelper
{
public:
    static QByteArray createUnwindInfo(Function*f, size_t functionSize);

    static void registerFunction(Function *function);
    static void registerFunctions(QVector<Function *> functions);
    static void deregisterFunction(Function *function);
    static void deregisterFunctions(QVector<Function *> functions);
};

} // VM namespace
} // QQmlJS namespace

#endif // QV4UNWINDHELPER_H
