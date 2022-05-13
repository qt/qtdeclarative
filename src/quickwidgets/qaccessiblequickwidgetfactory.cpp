// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequickwidgetfactory_p.h"
#include "qaccessiblequickwidget_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleInterface *qAccessibleQuickWidgetFactory(const QString &classname, QObject *object)
{
    if (classname == QLatin1String("QQuickWidget")) {
        return new QAccessibleQuickWidget(qobject_cast<QQuickWidget *>(object));
    } else if (classname == QLatin1String("QQuickWidgetOffscreenWindow")) {
        return new QAccessibleQuickWidgetOffscreenWindow(qobject_cast<QQuickWindow *>(object));
    }
    return 0;
}

#endif // accessibility

QT_END_NAMESPACE

