// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltyperegistrarutils_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QTypeRevision handleInMinorVersion(QTypeRevision revision, int majorVersion)
{
    if (!revision.hasMajorVersion() && revision.hasMinorVersion()) {
        // this version has been obtained by QML_{ADDED,REMOVED}_IN_MINOR_VERSION
        revision = QTypeRevision::fromVersion(majorVersion, revision.minorVersion());
    }
    return revision;
}


static QDebug message(QDebug base, QAnyStringView message, QAnyStringView fileName, int lineNumber)
{
    const QString lineString = lineNumber ? QString::number(lineNumber) : QString();
    return (base.noquote().nospace()
            << message << ": " << fileName << ":" << lineString << ":").space();
}

QDebug warning(QAnyStringView fileName, int lineNumber)
{
    return message(qWarning(), "Warning", fileName, lineNumber);
}

QDebug warning(const QCborMap &classDef)
{
    // TODO: Once we have line numbers, use them
    const QAnyStringView file = QAnyStringViewUtils::toStringView(
            classDef, Constants::MetatypesDotJson::S_INPUT_FILE);
    if (!file.isEmpty())
        return warning(file);

    const QAnyStringView name = QAnyStringViewUtils::toStringView(
            classDef, Constants::MetatypesDotJson::S_QUALIFIED_CLASS_NAME);
    return warning(name);
}

QDebug error(QAnyStringView fileName, int lineNumber)
{
    return message(qCritical(), "Error", fileName, lineNumber);
}

QT_END_NAMESPACE
