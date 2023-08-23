// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltyperegistrarutils_p.h"

QT_BEGIN_NAMESPACE

QTypeRevision handleInMinorVersion(QTypeRevision revision, int majorVersion)
{
    if (!revision.hasMajorVersion() && revision.hasMinorVersion()) {
        // this version has been obtained by QML_{ADDED,REMOVED}_IN_MINOR_VERSION
        revision = QTypeRevision::fromVersion(majorVersion, revision.minorVersion());
    }
    return revision;
}

QT_END_NAMESPACE
