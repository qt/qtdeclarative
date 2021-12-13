/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljscompilepass_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qv4compileddata_p.h>

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
    QQmlJSAotCompiler(QQmlJSImporter *importer, const QString &resourcePath,
                      const QStringList &qmldirFiles, QQmlJSLogger *logger);

    virtual ~QQmlJSAotCompiler() = default;

    virtual void setDocument(const QmlIR::JSCodeGen *codegen, const QmlIR::Document *document);
    virtual void setScope(const QmlIR::Object *object, const QmlIR::Object *scope);
    virtual std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> compileBinding(
            const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding);
    virtual std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> compileFunction(
            const QV4::Compiler::Context *context, const QmlIR::Function &irFunction);

    virtual QQmlJSAotFunction globalCode() const;

protected:
    virtual QQmlJS::DiagnosticMessage diagnose(
            const QString &message, QtMsgType type, const QQmlJS::SourceLocation &location) const;

    QQmlJSTypeResolver m_typeResolver;
    QStringList m_entireSourceCodeLines;

    const QString m_resourcePath;
    const QStringList m_qmldirFiles;

    const QmlIR::Document *m_document = nullptr;
    const QmlIR::Object *m_currentObject = nullptr;
    const QmlIR::Object *m_currentScope = nullptr;
    const QV4::Compiler::JSUnitGenerator *m_unitGenerator = nullptr;

    QQmlJSImporter *m_importer = nullptr;
    QQmlJSLogger *m_logger = nullptr;

private:
    QQmlJSAotFunction doCompile(
            const QV4::Compiler::Context *context, QQmlJSCompilePass::Function *function,
            QQmlJS::DiagnosticMessage *error);
};


using QQmlJSAotFunctionMap = QMap<int, QQmlJSAotFunction>;
using QQmlJSSaveFunction
    = std::function<bool(const QV4::CompiledData::SaveableUnitPointer &,
                         const QQmlJSAotFunctionMap &, QString *)>;

bool qCompileQmlFile(const QString &inputFileName, QQmlJSSaveFunction saveFunction,
                     QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error,
                     bool storeSourceLocation = false,
                     QV4::Compiler::CodegenWarningInterface *interface =
                             QV4::Compiler::defaultCodegenWarningInterface(),
                     const QString *fileContents = nullptr);
bool qCompileQmlFile(QmlIR::Document &irDocument, const QString &inputFileName,
                     QQmlJSSaveFunction saveFunction, QQmlJSAotCompiler *aotCompiler,
                     QQmlJSCompileError *error, bool storeSourceLocation = false,
                     QV4::Compiler::CodegenWarningInterface *interface =
                             QV4::Compiler::defaultCodegenWarningInterface(),
                     const QString *fileContents = nullptr);
bool qCompileJSFile(const QString &inputFileName, const QString &inputFileUrl,
                    QQmlJSSaveFunction saveFunction, QQmlJSCompileError *error);


bool qSaveQmlJSUnitAsCpp(const QString &inputFileName, const QString &outputFileName,
                         const QV4::CompiledData::SaveableUnitPointer &unit,
                         const QQmlJSAotFunctionMap &aotFunctions,
                         QString *errorString);

QT_END_NAMESPACE

#endif // QQMLJSCOMPILER_P_H
