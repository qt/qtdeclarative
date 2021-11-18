/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef CODEGEN_P_H
#define CODEGEN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QString>
#include <QFile>
#include <QList>

#include <variant>
#include <memory>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljscompiler_p.h>

#include <QtQmlCompiler/private/qqmljstyperesolver_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljscompilepass_p.h>

QT_BEGIN_NAMESPACE

class Codegen : public QQmlJSAotCompiler
{
public:
    Codegen(const QString &fileName, const QStringList &qmltypesFiles,
            QQmlJSLogger *logger, QQmlJSTypeInfo *typeInfo, const QString &m_code);

    void setDocument(const QmlIR::JSCodeGen *codegen, const QmlIR::Document *document) override;
    void setScope(const QmlIR::Object *object, const QmlIR::Object *scope) override;
    std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
    compileBinding(const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding) override;
    std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
    compileFunction(const QV4::Compiler::Context *context,
                    const QmlIR::Function &irFunction) override;

    QQmlJSAotFunction globalCode() const override;

    void setTypeResolver(std::unique_ptr<QQmlJSTypeResolver> typeResolver)
    {
        m_typeResolver = std::move(typeResolver);
    }

private:
    using Function = QQmlJSCompilePass::Function;

    const QmlIR::Document *m_document = nullptr;
    const QString m_fileName;
    const QStringList m_resourceFiles;
    const QStringList m_qmltypesFiles;

    const QmlIR::Object *m_currentObject = nullptr;
    QQmlJSScope::ConstPtr m_scopeType;
    QQmlJSScope::ConstPtr m_objectType;
    const QV4::Compiler::JSUnitGenerator *m_unitGenerator = nullptr;
    QStringList m_entireSourceCodeLines;
    QQmlJSLogger *m_logger;
    QQmlJSTypeInfo *m_typeInfo;
    const QString m_code;
    std::unique_ptr<QQmlJSTypeResolver> m_typeResolver;

    QQmlJS::DiagnosticMessage diagnose(const QString &message, QtMsgType type,
                                       const QQmlJS::SourceLocation &location);
    bool generateFunction(QV4::Compiler::ContextType contextType,
                          const QV4::Compiler::Context *context,
                          QQmlJS::AST::FunctionExpression *ast, Function *function,
                          QQmlJS::DiagnosticMessage *error) const;
};

QT_END_NAMESPACE

#endif
