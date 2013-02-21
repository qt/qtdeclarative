#ifndef QV4UNWINDHELPER_H
#define QV4UNWINDHELPER_H

#include <QtCore/QVector>

namespace QQmlJS {
namespace VM {

struct Function;

class UnwindHelper
{
    Q_DISABLE_COPY(UnwindHelper)

private:
    UnwindHelper();

public:
    ~UnwindHelper();

    static UnwindHelper *create();

    void registerFunction(Function *function);
    void registerFunctions(QVector<Function *> functions);
    void deregisterFunction(Function *function);
    void deregisterFunctions(QVector<Function *> functions);

private:
    struct Private;
    Private *p;
};

} // VM namespace
} // QQmlJS namespace

#endif // QV4UNWINDHELPER_H
