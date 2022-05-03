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

#ifndef QML2CPPCONTEXT_H
#define QML2CPPCONTEXT_H

#include "prototype/qmlcompiler.h"
#include "prototype/typeresolver.h"

#include <private/qqmljsdiagnosticmessage_p.h>
#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljstyperesolver_p.h>

#include <QtCore/QList>

#include <variant>
#include <functional>

struct Qml2CppContext
{
    const QmlIR::Document *document = nullptr;
    const Qmltc::TypeResolver *typeResolver = nullptr;
    QString documentUrl;
    QQmlJSLogger *logger = nullptr;
    const QHash<QQmlJSScope::ConstPtr, qsizetype> *typeIndices = nullptr; // TODO: remove this?

    void recordError(const QQmlJS::SourceLocation &location, const QString &message) const
    {
        Q_ASSERT(logger);
        logger->logCritical(message, Log_Compiler, location);
    }

    void recordError(const QV4::CompiledData::Location &location, const QString &message) const
    {
        recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message);
    }
};

struct Qml2CppObject
{
    QmlIR::Object *irObject = nullptr; // raw IR object representation
    QQmlJSScope::Ptr type; // Qml/JS type representation for IR object
};

// the corner stone of the compiler: compiler pass function prototype. the main
// idea is that, given "context" as input and object hierarchy as input/output,
// you run an arbitrary function (e.g. to verify that QQmlJSScope::ConstPtr
// types are valid).
using Qml2CppCompilerPass = std::function<void(/* IN */ const Qml2CppContext &,
                                               /* INOUT */ QList<Qml2CppObject> &)>;

class Qml2CppCompilerPassExecutor
{
    Qml2CppContext m_context;
    QList<Qml2CppObject> &m_objects;
    QList<Qml2CppCompilerPass> m_passes;

public:
    Qml2CppCompilerPassExecutor(const QmlIR::Document *doc, const Qmltc::TypeResolver *resolver,
                                const QString &url, QList<Qml2CppObject> &objects,
                                const QHash<QQmlJSScope::ConstPtr, qsizetype> &typeIndices)
        : m_context { doc, resolver, url, nullptr, &typeIndices }, m_objects { objects }
    {
    }

    // add new pass to the executor. first pass is typically the one that
    // populates the Qml2CppObject list
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

#endif // QML2CPPCONTEXT_H
