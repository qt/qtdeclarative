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

#include "codegen_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsshadowcheck_p.h>
#include <QtQmlCompiler/private/qqmljsstoragegeneralizer_p.h>
#include <QtQmlCompiler/private/qqmljstypepropagator_p.h>
#include <QtQmlCompiler/private/qqmljsfunctioninitializer_p.h>

#include <QFileInfo>

QT_BEGIN_NAMESPACE

Codegen::Codegen(QQmlJSImporter *importer, const QString &fileName,
                 const QStringList &qmldirFiles, QQmlJSLogger *logger, QQmlJSTypeInfo *typeInfo)
    : QQmlJSAotCompiler(importer, fileName, qmldirFiles, logger)
    , m_typeInfo(typeInfo)
{
}

void Codegen::setDocument(const QmlIR::JSCodeGen *codegen, const QmlIR::Document *document)
{
    Q_UNUSED(codegen);
    m_document = document;
    m_unitGenerator = &document->jsGenerator;
    m_entireSourceCodeLines = document->code.split(u'\n');
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
Codegen::compileBinding(const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding)
{
    QQmlJSFunctionInitializer initializer(&m_typeResolver, m_currentObject, m_currentScope);

    QQmlJS::DiagnosticMessage initializationError;
    const QString name = m_document->stringAt(irBinding.propertyNameIndex);
    QQmlJSCompilePass::Function function = initializer.run(
                context, name, irBinding, &initializationError);
    if (initializationError.isValid())
        diagnose(initializationError.message, initializationError.type, initializationError.loc);

    QQmlJS::DiagnosticMessage analyzeError;
    if (!analyzeFunction(context, &function, &analyzeError)) {
        // If it's a signal and the function just returns a closure, it's harmless.
        // Otherwise promote the message to warning level.
        return diagnose(u"Could not compile binding for %1: %2"_qs.arg(name, analyzeError.message),
                        (function.isSignalHandler && analyzeError.type == QtDebugMsg)
                            ? QtDebugMsg
                            : QtWarningMsg,
                        analyzeError.loc);
    }

    return QQmlJSAotFunction {};
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
Codegen::compileFunction(const QV4::Compiler::Context *context, const QmlIR::Function &irFunction)
{
    QQmlJS::DiagnosticMessage initializationError;
    QQmlJSFunctionInitializer initializer(&m_typeResolver, m_currentObject, m_currentScope);
    const QString name = m_document->stringAt(irFunction.nameIndex);
    QQmlJSCompilePass::Function function = initializer.run(
                context, name, irFunction, &initializationError);
    if (initializationError.isValid())
        diagnose(initializationError.message, initializationError.type, initializationError.loc);

    QQmlJS::DiagnosticMessage analyzeError;
    if (!analyzeFunction(context, &function, &analyzeError)) {
        return diagnose(u"Could not compile function %1: %2"_qs.arg(name, analyzeError.message),
                        QtWarningMsg, analyzeError.loc);
    }

    return QQmlJSAotFunction {};
}

bool Codegen::analyzeFunction(
        const QV4::Compiler::Context *context,
        QQmlJSCompilePass::Function *function,
        QQmlJS::DiagnosticMessage *error)
{
    QQmlJSTypePropagator propagator(m_unitGenerator, &m_typeResolver, m_logger, m_typeInfo);
    QQmlJSCompilePass::InstructionAnnotations annotations = propagator.run(function, error);
    if (!error->isValid()) {
        QQmlJSShadowCheck shadowCheck(m_unitGenerator, &m_typeResolver, m_logger);
        shadowCheck.run(&annotations, function, error);
    }

    if (!error->isValid()) {
        QQmlJSStorageGeneralizer generalizer(m_unitGenerator, &m_typeResolver, m_logger);
        generalizer.run(annotations, function, error);
    }

    if (error->isValid()) {
        error->type = context->returnsClosure ? QtDebugMsg : QtWarningMsg;
        return false;
    }

    return true;
}

QT_END_NAMESPACE
