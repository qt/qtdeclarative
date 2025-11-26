// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLEQUICKAPPLICATIONWINDOW_H
#define QACCESSIBLEQUICKAPPLICATIONWINDOW_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickapplicationwindow_p.h"

#include <QtGui/qaccessible.h>
#include <QtQuick/private/qaccessiblequickview_p.h>

QT_BEGIN_NAMESPACE

class QAccessibleQuickApplicationWindow : public QAccessibleQuickWindow,
                                          public QAccessibleAttributesInterface
{
public:
    QAccessibleQuickApplicationWindow(QQuickApplicationWindow *window);

    // QAccessibleInterface
    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleAttributesInterface
    virtual QList<QAccessible::Attribute> attributeKeys() const override;
    virtual QVariant attributeValue(QAccessible::Attribute key) const override;

private:
    QQuickApplicationWindow *applicationWindow() const;
};

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKAPPLICATIONWINDOW_H
