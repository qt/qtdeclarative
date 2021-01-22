/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef QV4SCRIPT_H
#define QV4SCRIPT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4global_p.h"
#include "qv4engine_p.h"
#include "qv4functionobject_p.h"
#include "qv4qmlcontext_p.h"
#include "private/qv4compilercontext_p.h"

#include <QQmlError>

QT_BEGIN_NAMESPACE

class QQmlContextData;

namespace QQmlJS {
class Engine;
}

namespace QV4 {

struct Q_QML_EXPORT Script {
    Script(ExecutionContext *scope, QV4::Compiler::ContextType mode, const QString &sourceCode, const QString &source = QString(), int line = 1, int column = 0)
        : sourceFile(source), line(line), column(column), sourceCode(sourceCode)
        , context(scope), strictMode(false), inheritContext(false), parsed(false), contextType(mode)
        , vmFunction(nullptr), parseAsBinding(false) {}
    Script(ExecutionEngine *engine, QmlContext *qml, bool parseAsBinding, const QString &sourceCode, const QString &source = QString(), int line = 1, int column = 0)
        : sourceFile(source), line(line), column(column), sourceCode(sourceCode)
        , context(engine->rootContext()), strictMode(false), inheritContext(true), parsed(false)
        , vmFunction(nullptr), parseAsBinding(parseAsBinding) {
        if (qml)
            qmlContext.set(engine, *qml);
    }
    Script(ExecutionEngine *engine, QmlContext *qml, const QQmlRefPointer<ExecutableCompilationUnit> &compilationUnit);
    ~Script();
    QString sourceFile;
    int line;
    int column;
    QString sourceCode;
    ExecutionContext *context;
    bool strictMode;
    bool inheritContext;
    bool parsed;
    QV4::Compiler::ContextType contextType = QV4::Compiler::ContextType::Eval;
    QV4::PersistentValue qmlContext;
    QQmlRefPointer<ExecutableCompilationUnit> compilationUnit;
    Function *vmFunction;
    bool parseAsBinding;

    void parse();
    ReturnedValue run(const QV4::Value *thisObject = nullptr);

    Function *function();

    static QV4::CompiledData::CompilationUnit precompile(
            QV4::Compiler::Module *module, QQmlJS::Engine *jsEngine,
            Compiler::JSUnitGenerator *unitGenerator, const QString &fileName,
            const QString &finalUrl, const QString &source,
            QList<QQmlError> *reportedErrors = nullptr,
            QV4::Compiler::ContextType contextType = QV4::Compiler::ContextType::Global);
    static Script *createFromFileOrCache(ExecutionEngine *engine, QmlContext *qmlContext, const QString &fileName, const QUrl &originalUrl, QString *error);
};

}

QT_END_NAMESPACE

#endif
