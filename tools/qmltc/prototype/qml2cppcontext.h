// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QML2CPPCONTEXT_H
#define QML2CPPCONTEXT_H

#include "qmltctyperesolver.h"

#include <private/qqmljsdiagnosticmessage_p.h>
#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljstyperesolver_p.h>

#include <QtCore/QList>

#include <variant>
#include <functional>

QT_BEGIN_NAMESPACE

struct Qml2CppContext
{
    const QmltcTypeResolver *typeResolver = nullptr;
    QString documentUrl;
    QQmlJSLogger *logger = nullptr;

    void recordError(const QQmlJS::SourceLocation &location, const QString &message) const
    {
        Q_ASSERT(logger);
        logger->log(message, Log_Compiler, location);
    }

    void recordError(const QV4::CompiledData::Location &location, const QString &message) const
    {
        recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message);
    }
};

// the corner stone of the compiler: compiler pass function prototype. the main
// idea is that, given "context" as input and object hierarchy as input/output,
// you run an arbitrary function (e.g. to verify that QQmlJSScope::ConstPtr
// types are valid).
using Qml2CppCompilerPass = std::function<void(/* IN */ const Qml2CppContext &,
                                               /* INOUT */ QList<QQmlJSScope::Ptr> &)>;

class Qml2CppCompilerPassExecutor
{
    Qml2CppContext m_context;
    QList<QQmlJSScope::Ptr> &m_objects;
    QList<Qml2CppCompilerPass> m_passes;

public:
    Qml2CppCompilerPassExecutor(const QmltcTypeResolver *resolver, const QString &url,
                                QList<QQmlJSScope::Ptr> &objects)
        : m_context { resolver, url }, m_objects { objects }
    {
    }

    // add new pass to the executor. first pass is typically the one that
    // populates the QQmlJSScope::Ptr list
    void addPass(Qml2CppCompilerPass pass) { m_passes.append(pass); }

    // runs passes in the order of their insertion, populating \a logger on
    // errors. right now always stops if new errors are found
    void run(QQmlJSLogger *logger)
    {
        m_context.logger = logger;
        for (const auto &pass : m_passes) {
            pass(m_context, m_objects);
            if (m_context.logger->hasErrors())
                return;
        }
    }
};

QT_END_NAMESPACE

#endif // QML2CPPCONTEXT_H
