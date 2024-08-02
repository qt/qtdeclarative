// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljslintercodegen_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsshadowcheck_p.h>
#include <QtQmlCompiler/private/qqmljsstoragegeneralizer_p.h>
#include <QtQmlCompiler/private/qqmljsstorageinitializer_p.h>
#include <QtQmlCompiler/private/qqmljstypepropagator_p.h>
#include <QtQmlCompiler/private/qqmljsfunctioninitializer_p.h>

#include <QFileInfo>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSLinterCodegen::QQmlJSLinterCodegen(QQmlJSImporter *importer, const QString &fileName,
                                         const QStringList &qmldirFiles, QQmlJSLogger *logger)
    : QQmlJSAotCompiler(importer, fileName, qmldirFiles, logger)
{
}

void QQmlJSLinterCodegen::setDocument(const QmlIR::JSCodeGen *codegen,
                                      const QmlIR::Document *document)
{
    Q_UNUSED(codegen);
    m_document = document;
    m_unitGenerator = &document->jsGenerator;
}

std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>>
QQmlJSLinterCodegen::compileBinding(const QV4::Compiler::Context *context,
                                    const QmlIR::Binding &irBinding, QQmlJS::AST::Node *astNode)
{
    QQmlJSFunctionInitializer initializer(
                &m_typeResolver, m_currentObject->location, m_currentScope->location);

    QList<QQmlJS::DiagnosticMessage> initializationErrors;
    const QString name = m_document->stringAt(irBinding.propertyNameIndex);
    QQmlJSCompilePass::Function function =
            initializer.run(context, name, astNode, irBinding, &initializationErrors);
    for (const auto &error : initializationErrors)
        diagnose(error.message, error.type, error.loc);

    QList<QQmlJS::DiagnosticMessage> analyzeErrors;
    if (!analyzeFunction(context, &function, &analyzeErrors)) {
        // If it's a signal and the function just returns a closure, it's harmless.
        // Otherwise promote the message to warning level.
        for (auto &error : analyzeErrors) {
            error = diagnose(u"Could not compile binding for %1: %2"_s.arg(name, error.message),
                             (function.isSignalHandler && error.type == QtDebugMsg)
                                     ? QtDebugMsg
                                     : QtWarningMsg,
                             error.loc);
        }
        return analyzeErrors;
    }

    return QQmlJSAotFunction {};
}

std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>>
QQmlJSLinterCodegen::compileFunction(const QV4::Compiler::Context *context,
                                     const QString &name, QQmlJS::AST::Node *astNode)
{
    QList<QQmlJS::DiagnosticMessage> initializationErrors;
    QQmlJSFunctionInitializer initializer(
                &m_typeResolver, m_currentObject->location, m_currentScope->location);
    QQmlJSCompilePass::Function function =
            initializer.run(context, name, astNode, &initializationErrors);
    for (const auto &error : initializationErrors)
        diagnose(error.message, error.type, error.loc);

    QList<QQmlJS::DiagnosticMessage> analyzeErrors;
    if (!analyzeFunction(context, &function, &analyzeErrors)) {
        for (auto &error : analyzeErrors) {
            error = diagnose(u"Could not compile function %1: %2"_s.arg(name, error.message),
                             QtWarningMsg, error.loc);
        }
        return analyzeErrors;
    }

    return QQmlJSAotFunction {};
}

void QQmlJSLinterCodegen::setPassManager(QQmlSA::PassManager *passManager)
{
    m_passManager = passManager;
    auto managerPriv = QQmlSA::PassManagerPrivate::get(passManager);
    managerPriv->m_typeResolver = typeResolver();
}

bool QQmlJSLinterCodegen::analyzeFunction(const QV4::Compiler::Context *context,
                                          QQmlJSCompilePass::Function *function,
                                          QList<QQmlJS::DiagnosticMessage> *errors)
{
    QQmlJSTypePropagator propagator(m_unitGenerator, &m_typeResolver, m_logger, errors, {}, {},
                                    m_passManager);
    auto [basicBlocks, annotations] = propagator.run(function);
    if (errors->isEmpty()) {
        QQmlJSShadowCheck shadowCheck(m_unitGenerator, &m_typeResolver, m_logger, errors,
                                      basicBlocks, annotations);
        shadowCheck.run(function);
    }

    if (errors->isEmpty()) {
        QQmlJSStorageInitializer initializer(m_unitGenerator, &m_typeResolver, m_logger, errors,
                                             basicBlocks, annotations);
        initializer.run(function);
    }

    if (errors->isEmpty()) {
        QQmlJSStorageGeneralizer generalizer(m_unitGenerator, &m_typeResolver, m_logger, errors,
                                             basicBlocks, annotations);
        generalizer.run(function);
    }

    if (!errors->isEmpty()) {
        QtMsgType type = context->returnsClosure ? QtDebugMsg : QtWarningMsg;
        for (auto &error : *errors)
            error.type = type;
        return false;
    }

    return true;
}

QT_END_NAMESPACE
