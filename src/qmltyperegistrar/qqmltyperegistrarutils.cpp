// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltyperegistrarutils_p.h"

#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qmetatypesjsonprocessor_p.h"

#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qcborvalue.h>
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

QAnyStringView interfaceName(const Interface &iface)
{
    return iface.className;
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

QDebug warning(const MetaType &classDef)
{
    // TODO: Once we have line numbers, use them
    const QAnyStringView file = classDef.inputFile();
    if (!file.isEmpty())
        return warning(file);

    return warning(classDef.qualifiedClassName());
}

QDebug error(QAnyStringView fileName, int lineNumber)
{
    return message(qCritical(), "Error", fileName, lineNumber);
}

QT_END_NAMESPACE
