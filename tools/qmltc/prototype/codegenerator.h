// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "qmltctyperesolver.h"
#include "qmltcoutputir.h"
#include "prototype/qml2cppcontext.h"

#include <QtCore/qlist.h>
#include <QtCore/qqueue.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>

#include <variant>
#include <utility>

QT_BEGIN_NAMESPACE

struct QmltcCompilerInfo;
class CodeGenerator
{
public:
    CodeGenerator(const QString &url, QQmlJSLogger *logger, const QmltcTypeResolver *localResolver,
                  const QmltcVisitor *visitor, const QmltcCompilerInfo *info);

    // initializes code generator
    void prepare(QSet<QString> *requiredCppIncludes,
                 const QSet<QQmlJSScope::ConstPtr> &suitableTypes);

private:
    QString m_url; // document url
    QQmlJSLogger *m_logger = nullptr;
    const QmltcTypeResolver *m_localTypeResolver = nullptr;
    const QmltcVisitor *m_visitor = nullptr;

    const QmltcCompilerInfo *m_info = nullptr;

private:
    // helper methods:
    void recordError(const QQmlJS::SourceLocation &location, const QString &message);
    void recordError(const QV4::CompiledData::Location &location, const QString &message);
};

QT_END_NAMESPACE

#endif // CODEGENERATOR_H
