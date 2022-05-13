// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtquicktemplates2global_p.h"

#include <QtGui/qtguiglobal.h>

#if QT_CONFIG(accessibility)
#include "qquickpage_p.h"
#include "accessible/qaccessiblequickpage_p.h"
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)
static QAccessibleInterface *qQuickAccessibleFactory(const QString &classname, QObject *object)
{
    if (classname == u"QQuickPage") {
        return new QAccessibleQuickPage(qobject_cast<QQuickPage *>(object));
    }
    return nullptr;
}
#endif

void QQuickTemplates_initializeModule()
{
#if QT_CONFIG(accessibility)
    QAccessible::installFactory(&qQuickAccessibleFactory);
#endif
}

Q_CONSTRUCTOR_FUNCTION(QQuickTemplates_initializeModule)

QT_END_NAMESPACE
