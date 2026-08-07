// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEQUICKWINDOWCONTAINER_P_H
#define QACCESSIBLEQUICKWINDOWCONTAINER_P_H

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

#include "qaccessiblequickitem_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QQuickWindowContainer;

class Q_QUICK_EXPORT QAccessibleQuickWindowContainer : public QAccessibleQuickItem
{
public:
    QAccessibleQuickWindowContainer(QQuickWindowContainer *container);

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;

protected:
    QQuickWindowContainer *container() const;
    QAccessibleInterface *accessibleRoot() const;
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKWINDOWCONTAINER_P_H
