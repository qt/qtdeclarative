// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEQUICKLISTVIEW_H
#define QACCESSIBLEQUICKLISTVIEW_H

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

class QQuickListView;

class Q_QUICK_EXPORT QAccessibleQuickListView : public QAccessibleQuickItem,
                                                public QAccessibleViewportInterface
{
public:
    QAccessibleQuickListView(QQuickListView *listView);

    // Viewport Interface
    QSizeF contentSize() const override;
    QPointF position() const override;
    QSizeF viewportSize() const override;
    bool isIndexed() const override;
    void setPosition(QPointF position) override;

protected:
    QQuickListView *listView() const;
    void *interface_cast(QAccessible::InterfaceType t) override;
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKLISTVIEW_H
