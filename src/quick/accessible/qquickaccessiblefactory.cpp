// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
