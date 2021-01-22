/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickaccessiblefactory_p.h"

#include "qaccessiblequickview_p.h"
#include "qaccessiblequickitem_p.h"
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE
#if QT_CONFIG(accessibility)

QAccessibleInterface *qQuickAccessibleFactory(const QString &classname, QObject *object)
{
    if (classname == QLatin1String("QQuickWindow")) {
        return new QAccessibleQuickWindow(qobject_cast<QQuickWindow *>(object));
    } else if (classname == QLatin1String("QQuickItem")) {
        QQuickItem *item = qobject_cast<QQuickItem *>(object);
        Q_ASSERT(item);
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
        if (!itemPrivate->isAccessible)
            return nullptr;
        return new QAccessibleQuickItem(item);
    }

    return nullptr;
}

#endif
QT_END_NAMESPACE
