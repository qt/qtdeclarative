// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslintercodegen_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsshadowcheck_p.h>
#include <QtQmlCompiler/private/qqmljsstoragegeneralizer_p.h>
#include <QtQmlCompiler/private/qqmljsstorageinitializer_p.h>
#include <QtQmlCompiler/private/qqmljsfunctioninitializer_p.h>
#include <QtQmlCompiler/private/qqmljsbasicblocks_p.h>

#include <QFileInfo>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSLinterCodegen::QQmlJSLinterCodegen(QQmlJSImporter *importer, const QString &fileName,
                                         const QStringList &qmldirFiles, QQmlJSLogger *logger,
                                         const ContextPropertyInfo &contextPropertyInfo)
    : QQmlJSAotCompiler(importer, fileName, qmldirFiles, logger),
      m_contextPropertyInfo(contextPropertyInfo)
{
    m_flags |= QQmlJSAotCompiler::IsLintCompiler;
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
    const QString name = m_document->stringAt(irBinding.propertyNameIndex);
    m_logger->setCompileErrorPrefix(
            u"Could not determine signature of binding for %1: "_s.arg(name));

    QQmlJSFunctionInitializer initializer(
                &m_typeResolver, m_currentObject->location, m_currentScope->location, m_logger);
    QQmlJSCompilePass::Function function = initializer.run(context, name, astNode, irBinding);

    m_logger->iterateCurrentFunctionMessages([this](const Message &error) {
        diagnose(error.message, error.type, error.loc);
    });

    m_logger->setCompileErrorPrefix(u"Could not compile binding for %1: "_s.arg(name));
    m_logger->setCompileSkipPrefix(u"Compilation of binding for %1 was skipped: "_s.arg(name));

    analyzeFunction(context, &function);
    if (const auto errors = finalizeBindingOrFunction())
        return *errors;

    return QQmlJSAotFunction {};
}

std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>>
QQmlJSLinterCodegen::compileFunction(const QV4::Compiler::Context *context,
                                     const QString &name, QQmlJS::AST::Node *astNode)
{
    m_logger->setCompileErrorPrefix(u"Could not determine signature of function %1: "_s.arg(name));

    QQmlJSFunctionInitializer initializer(
                &m_typeResolver, m_currentObject->location, m_currentScope->location, m_logger);
    QQmlJSCompilePass::Function function = initializer.run(context, name, astNode);

    m_logger->iterateCurrentFunctionMessages([this](const Message &error) {
        diagnose(error.message, error.type, error.loc);
    });

    m_logger->setCompileErrorPrefix(u"Could not compile function %1: "_s.arg(name));
    m_logger->setCompileSkipPrefix(u"Compilation of function %1 was skipped: "_s.arg(name));
    analyzeFunction(context, &function);

    if (const auto errors = finalizeBindingOrFunction())
        return *errors;

    return QQmlJSAotFunction {};
}

void QQmlJSLinterCodegen::setPassManager(QQmlSA::PassManager *passManager)
{
    m_passManager = passManager;
    auto managerPriv = QQmlSA::PassManagerPrivate::get(passManager);
    managerPriv->m_typeResolver = typeResolver();
}

void QQmlJSLinterCodegen::analyzeFunction(const QV4::Compiler::Context *context,
                                          QQmlJSCompilePass::Function *function)
{
    bool dummy = false;
    QQmlJSCompilePass::BlocksAndAnnotations blocksAndAnnotations =
            QQmlJSBasicBlocks(context, m_unitGenerator, &m_typeResolver, m_logger)
                    .run(function, ValidateBasicBlocks, dummy);

    QQmlJSTypePropagator lintTypePropgator(
            m_unitGenerator, &m_typeResolver, m_logger, blocksAndAnnotations.basicBlocks,
            blocksAndAnnotations.annotations, m_passManager, m_contextPropertyInfo);
    lintTypePropgator.setKnownUnresolvedTypes(m_knownUnresolvedTypes);
    blocksAndAnnotations = lintTypePropgator.run(function);

    if (m_logger->isCategoryIgnored(qmlCompiler))
        return;

    if (!m_logger->currentFunctionHasCompileError()) {
        blocksAndAnnotations = QQmlJSShadowCheck(m_unitGenerator, &m_typeResolver, m_logger,
                                                 blocksAndAnnotations.basicBlocks,
                                                 blocksAndAnnotations.annotations)
                                       .run(function);
    }

    if (!m_logger->currentFunctionHasCompileError()) {
        blocksAndAnnotations = QQmlJSStorageInitializer(m_unitGenerator, &m_typeResolver, m_logger,
                                                        blocksAndAnnotations.basicBlocks,
                                                        blocksAndAnnotations.annotations)
                                       .run(function);
    }

    if (!m_logger->currentFunctionHasCompileError()) {
        blocksAndAnnotations = QQmlJSStorageGeneralizer(m_unitGenerator, &m_typeResolver, m_logger,
                                                        blocksAndAnnotations.basicBlocks,
                                                        blocksAndAnnotations.annotations)
                                       .run(function);
    }
}

QT_END_NAMESPACE
