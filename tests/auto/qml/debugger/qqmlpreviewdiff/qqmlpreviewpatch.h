// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLPREVIEWPATCH_H
#define QQMLPREVIEWPATCH_H

#include <QtCore/qbytearray.h>
#include <private/qqmlpreviewdiff_p.h>

QT_BEGIN_NAMESPACE

namespace QV4::CompiledData {

// Apply a diff to an old compilation unit to produce a new one.
// The diff must contain all information needed to reconstruct the changes.
// Returns a QByteArray containing the patched unit data.
// On failure, returns an empty QByteArray and sets errorMessage if provided.
QByteArray patchCompilationUnit(const Unit *oldUnit, const CompilationUnitDiff &diff,
                                QString *errorMessage = nullptr);

} // namespace QV4::CompiledData

QT_END_NAMESPACE

#endif // QQMLPREVIEWPATCH_H
