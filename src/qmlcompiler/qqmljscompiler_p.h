/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQMLJSCOMPILER_P_H
#define QQMLJSCOMPILER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmlirbuilder_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

struct QQmlJSCompileError
{
    QString message;
    void print();
    QQmlJSCompileError augment(const QString &contextErrorMessage) const;
    void appendDiagnostics(const QString &inputFileName,
                           const QList<QQmlJS::DiagnosticMessage> &diagnostics);
    void appendDiagnostic(const QString &inputFileName,
                          const QQmlJS::DiagnosticMessage &diagnostic);
};

struct QQmlJSAotFunction
{
    QStringList includes;
    QStringList argumentTypes;
    QString code;
    QString returnType;
};

class QQmlJSAotCompiler
{
public:
    virtual ~QQmlJSAotCompiler() = default;

    virtual void setDocument(QmlIR::JSCodeGen *codegen, QmlIR::Document *document) = 0;
    virtual void setScope(const QmlIR::Object *object, const QmlIR::Object *scope) = 0;
    virtual std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> compileBinding(
            const QV4::Compiler::Context *context,
            const QmlIR::Binding &binding) = 0;
    virtual std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> compileFunction(
            const QV4::Compiler::Context *context,
            const QmlIR::Function &function) = 0;

    virtual QQmlJSAotFunction globalCode() const = 0;
};


using QQmlJSAotFunctionMap = QMap<int, QQmlJSAotFunction>;
using QQmlJSSaveFunction
    = std::function<bool(const QV4::CompiledData::SaveableUnitPointer &,
                         const QQmlJSAotFunctionMap &, QString *)>;

bool qCompileQmlFile(const QString &inputFileName, QQmlJSSaveFunction saveFunction,
                     QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error);
bool qCompileQmlFile(QmlIR::Document &irDocument, const QString &inputFileName,
                     QQmlJSSaveFunction saveFunction, QQmlJSAotCompiler *aotCompiler,
                     QQmlJSCompileError *error);
bool qCompileJSFile(const QString &inputFileName, const QString &inputFileUrl,
                    QQmlJSSaveFunction saveFunction, QQmlJSCompileError *error);


bool qSaveQmlJSUnitAsCpp(const QString &inputFileName, const QString &outputFileName,
                         const QV4::CompiledData::SaveableUnitPointer &unit,
                         const QQmlJSAotFunctionMap &aotFunctions,
                         QString *errorString);

QT_END_NAMESPACE

#endif // QQMLJSCOMPILER_P_H
