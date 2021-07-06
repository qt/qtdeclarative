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

#ifndef CODEGEN_H
#define CODEGEN_H

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

class Codegen : public QQmlJSAotCompiler
{
public:
    Codegen(QQmlJSImporter *importer, const QString &fileName, const QStringList &qmltypesFiles,
            QQmlJSLogger *logger, const QString &m_code);

    void setDocument(QmlIR::JSCodeGen *codegen, QmlIR::Document *document) override;
    void setScope(const QmlIR::Object *object, const QmlIR::Object *scope) override;
    std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
    compileBinding(const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding) override;
    std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
    compileFunction(const QV4::Compiler::Context *context,
                    const QmlIR::Function &irFunction) override;

    QQmlJSAotFunction globalCode() const override;

private:
    struct Function
    {
        QV4::Compiler::ContextType contextType = QV4::Compiler::ContextType::Binding;
        QQmlJS::AST::FunctionExpression *ast = nullptr;
        bool isQPropertyBinding = false;

        QQmlJSScope::ConstPtr returnType;
        QList<QQmlJSScope::ConstPtr> argumentTypes;
        QQmlJSScope::ConstPtr qmlScope;

        QString generatedCode;
        QStringList includes;
        QQmlJS::DiagnosticMessage error;
    };

    const QmlIR::Document *m_document = nullptr;
    const QString m_fileName;
    const QStringList m_resourceFiles;
    const QStringList m_qmltypesFiles;
    QQmlJSImporter *m_importer = nullptr;

    QQmlJS::MemoryPool *m_pool = nullptr;
    const QmlIR::Object *m_currentObject = nullptr;
    QQmlJSScope::ConstPtr m_scopeType;
    QQmlJSScope::ConstPtr m_objectType;
    QV4::Compiler::JSUnitGenerator *m_unitGenerator = nullptr;
    QStringList m_entireSourceCodeLines;
    QQmlJSLogger *m_logger;
    const QString &m_code;
    std::unique_ptr<QQmlJSTypeResolver> m_typeResolver;

    QQmlJS::DiagnosticMessage diagnose(const QString &message, QtMsgType type,
                                       const QQmlJS::SourceLocation &location);
    bool generateFunction(const QV4::Compiler::Context *context, Function *function) const;
    void instructionOffsetToSrcLocation(const QV4::Compiler::Context *context, uint offset,
                                        QQmlJS::SourceLocation *srcLoc) const;
};

#endif
