/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QQMLMODULEREGISTRATION_H
#define QQMLMODULEREGISTRATION_H

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

struct QQmlModuleRegistrationPrivate;
class Q_QML_EXPORT QQmlModuleRegistration
{
    Q_DISABLE_COPY_MOVE(QQmlModuleRegistration)
public:
    QQmlModuleRegistration(const char *uri, int majorVersion, void (*registerFunction)());
    ~QQmlModuleRegistration();

private:
    QQmlModuleRegistrationPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLMODULEREGISTRATION_H
