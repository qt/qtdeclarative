// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPECOMPILER_P_H
#define QQMLTYPECOMPILER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <qqmlerror.h>
#include <qhash.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlpropertyresolver_p.h>
#include <private/qqmlpropertycachecreator_p.h>

QT_BEGIN_NAMESPACE

class QQmlEnginePrivate;
class QQmlError;
class QQmlTypeData;
class QQmlImports;
class QTypeRevision;

namespace QmlIR {
struct Document;
}

namespace QV4 {
namespace CompiledData {
struct QmlUnit;
struct Location;
}
}

struct QQmlTypeCompiler
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeCompiler)
public:
    QQmlTypeCompiler(QQmlTypeData *typeData, QmlIR::Document *document,
                     const QV4::CompiledData::DependentTypesHasher &dependencyHasher);

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> compile();

    QList<QQmlError> compilationErrors() const { return errors; }

    static QList<QQmlError> computeRuntimeData(
            QV4::CompiledData::CompilationUnit *unit, QQmlTypeLoader *typeLoader,
            const QQmlImports *imports);

private:
    void recordError(const QQmlJS::DiagnosticMessage &message);

    QList<QQmlError> errors;
    const QV4::CompiledData::DependentTypesHasher &dependencyHasher;
    QmlIR::Document *document;
    QQmlTypeData *typeData;
};

QT_END_NAMESPACE

#endif // QQMLTYPECOMPILER_P_H
