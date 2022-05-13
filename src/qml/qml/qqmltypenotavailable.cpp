// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypenotavailable_p.h"

QT_BEGIN_NAMESPACE

int qmlRegisterTypeNotAvailable(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& message)
{
    return qmlRegisterUncreatableType<QQmlTypeNotAvailable>(uri,versionMajor,versionMinor,qmlName,message);
}

QT_END_NAMESPACE

#include "moc_qqmltypenotavailable_p.cpp"
