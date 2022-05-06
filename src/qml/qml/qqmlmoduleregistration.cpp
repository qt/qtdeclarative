/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
