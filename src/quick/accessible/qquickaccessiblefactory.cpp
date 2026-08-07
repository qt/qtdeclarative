// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickaccessiblefactory_p.h"

#include "qaccessiblequickview_p.h"
#include "qaccessiblequickitem_p.h"
#include "qaccessiblequicktextedit_p.h"
#include "qaccessiblequicktextinput_p.h"
#include "qaccessiblequickflickable_p.h"
#include "qaccessiblequicklistview_p.h"
#include "qaccessiblequickwindowcontainer_p.h"
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquicktextinput_p.h>
#include <QtQuick/private/qquickwindowcontainer_p.h>

QT_BEGIN_NAMESPACE
#if QT_CONFIG(accessibility)

QAccessibleInterface *qQuickAccessibleFactory(const QString &classname, QObject *object)
{
    if (classname == QLatin1String("QQuickWindow"))
        return new QAccessibleQuickWindow(qobject_cast<QQuickWindow *>(object));
    if (classname == QLatin1String("QQuickTextEdit"))
        return new QAccessibleQuickTextEdit(qobject_cast<QQuickTextEdit *>(object));
    if (classname == QLatin1String("QQuickTextInput"))
        return new QAccessibleQuickTextInput(qobject_cast<QQuickTextInput *>(object));
    if (classname == QLatin1String("QQuickListView"))
        return new QAccessibleQuickListView(qobject_cast<QQuickListView *>(object));
    if (classname == QLatin1String("QQuickFlickable"))
        return new QAccessibleQuickFlickable(qobject_cast<QQuickFlickable *>(object));
    if (classname == QLatin1String("QQuickWindowContainer"))
        return new QAccessibleQuickWindowContainer(qobject_cast<QQuickWindowContainer *>(object));
    if (classname == QLatin1String("QQuickItem")) {
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
