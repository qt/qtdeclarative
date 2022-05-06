/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKSWIPEDELEGATE_P_P_H
#define QQUICKSWIPEDELEGATE_P_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p_p.h>
#include <QtQuickTemplates2/private/qquickswipe_p.h>

QT_BEGIN_NAMESPACE

class QQuickSwipeDelegate;

class QQuickSwipeDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipeDelegate)

public:
    QQuickSwipeDelegatePrivate(QQuickSwipeDelegate *control);

    bool handleMousePressEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event);
    void forwardMouseEvent(QMouseEvent *event, QQuickItem *destination, QPointF localPos);
    bool attachedObjectsSetPressed(QQuickItem *item, QPointF scenePos, bool pressed, bool cancel = false);

    void resizeContent() override;
    void resizeBackground() override;

    QPalette defaultPalette() const override;

    QQuickSwipe swipe;
    QQuickItem *pressedItem = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSWIPEDELEGATE_P_P_H
