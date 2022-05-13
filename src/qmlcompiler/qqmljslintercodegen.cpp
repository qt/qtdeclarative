// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljslintercodegen_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsshadowcheck_p.h>
#include <QtQmlCompiler/private/qqmljsstoragegeneralizer_p.h>
#include <QtQmlCompiler/private/qqmljstypepropagator_p.h>
#include <QtQmlCompiler/private/qqmljsfunctioninitializer_p.h>

#include <QFileInfo>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSLinterCodegen::QQmlJSLinterCodegen(QQmlJSImporter *importer, const QString &fileName,
                                         const QStringList &qmldirFiles, QQmlJSLogger *logger,
                                         QQmlJSTypeInfo *typeInfo)
    : QQmlJSAotCompiler(importer, fileName, qmldirFiles, logger), m_typeInfo(typeInfo)
{
}

void QQmlJSLinterCodegen::setDocument(const QmlIR::JSCodeGen *codegen,
                                      const QmlIR::Document *document)
{
    Q_UNUSED(codegen);
    m_document = document;
    m_unitGenerator = &document->jsGenerator;
    m_entireSourceCodeLines = document->code.split(u'\n');
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
QQmlJSLinterCodegen::compileBinding(const QV4::Compiler::Context *context,
                                    const QmlIR::Binding &irBinding)
{
    QQmlJSFunctionInitializer initializer(&m_typeResolver, m_currentObject, m_currentScope);

    QQmlJS::DiagnosticMessage initializationError;
    const QString name = m_document->stringAt(irBinding.propertyNameIndex);
    QQmlJSCompilePass::Function function =
            initializer.run(context, name, irBinding, &initializationError);
    if (initializationError.isValid())
        diagnose(initializationError.message, initializationError.type, initializationError.loc);

    QQmlJS::DiagnosticMessage analyzeError;
    if (!analyzeFunction(context, &function, &analyzeError)) {
        // If it's a signal and the function just returns a closure, it's harmless.
        // Otherwise promote the message to warning level.
        return diagnose(u"Could not compile binding for %1: %2"_s.arg(name, analyzeError.message),
                        (function.isSignalHandler && analyzeError.type == QtDebugMsg)
                            ? QtDebugMsg
                            : QtWarningMsg,
                        analyzeError.loc);
    }

    return QQmlJSAotFunction {};
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
QQmlJSLinterCodegen::compileFunction(const QV4::Compiler::Context *context,
                                     const QmlIR::Function &irFunction)
{
    QQmlJS::DiagnosticMessage initializationError;
    QQmlJSFunctionInitializer initializer(&m_typeResolver, m_currentObject, m_currentScope);
    const QString name = m_document->stringAt(irFunction.nameIndex);
    QQmlJSCompilePass::Function function =
            initializer.run(context, name, irFunction, &initializationError);
    if (initializationError.isValid())
        diagnose(initializationError.message, initializationError.type, initializationError.loc);

    QQmlJS::DiagnosticMessage analyzeError;
    if (!analyzeFunction(context, &function, &analyzeError)) {
        return diagnose(u"Could not compile function %1: %2"_s.arg(name, analyzeError.message),
                        QtWarningMsg, analyzeError.loc);
    }

    return QQmlJSAotFunction {};
}

bool QQmlJSLinterCodegen::analyzeFunction(const QV4::Compiler::Context *context,
                                          QQmlJSCompilePass::Function *function,
                                          QQmlJS::DiagnosticMessage *error)
{
    QQmlJSTypePropagator propagator(m_unitGenerator, &m_typeResolver, m_logger, m_typeInfo,
                                    m_passManager);
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
