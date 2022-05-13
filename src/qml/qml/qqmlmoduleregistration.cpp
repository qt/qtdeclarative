// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/qqmlmoduleregistration.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

struct QQmlModuleRegistrationPrivate
{
    const QString uri;
};

QQmlModuleRegistration::QQmlModuleRegistration(const char *uri, void (*registerFunction)()) :
    d(new QQmlModuleRegistrationPrivate { QString::fromUtf8(uri) })
{
    QQmlMetaType::qmlInsertModuleRegistration(d->uri, registerFunction);
}

#if QT_DEPRECATED_SINCE(6, 0)
QQmlModuleRegistration::QQmlModuleRegistration(
        const char *uri, int majorVersion, void (*registerFunction)()) :
    QQmlModuleRegistration(uri, registerFunction)
{
    Q_UNUSED(majorVersion);
}
#endif

QQmlModuleRegistration::~QQmlModuleRegistration()
{
    QQmlMetaType::qmlRemoveModuleRegistration(d->uri);
    delete d;
}

QT_END_NAMESPACE
