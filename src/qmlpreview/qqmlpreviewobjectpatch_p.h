// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLPREVIEWOBJECTPATCH_P_H
#define QQMLPREVIEWOBJECTPATCH_P_H

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

#include <private/qqmlpreviewdiff_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtQml/qqmlengine.h>
#include <QtCore/qobject.h>

#include <vector>

QT_BEGIN_NAMESPACE

namespace QQmlPreview {

bool applyDiff(std::vector<QObject *> &objects, const QV4::CompiledData::CompilationUnitDiff &diff,
               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit);

void refreshBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit);

} // namespace QQmlPreview

QT_END_NAMESPACE

#endif // QQMLPREVIEWOBJECTPATCH_P_H
