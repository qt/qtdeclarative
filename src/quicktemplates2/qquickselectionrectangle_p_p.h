/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKSELECTIONRECTANGLE_P_P_H
#define QQUICKSELECTIONRECTANGLE_P_P_H

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

#include "qquickselectionrectangle_p.h"

#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

#include <QtQuick/private/qquickselectable_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/private/qquickdraghandler_p.h>

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickSelectionRectanglePrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickSelectionRectangle)

public:
    QQuickSelectionRectanglePrivate();

    void updateDraggingState(bool isDragging);
    void updateActiveState(bool isActive);
    void updateHandles();
    void updateSelectionMode();
    void connectToTarget();
    void scrollTowardsPos(const QPointF &pos);
    QQuickItem *handleUnderPos(const QPointF &pos);

    QQuickItem *createHandle(QQmlComponent *delegate, Qt::Corner corner);

    QQuickSelectionRectangleAttached *getAttachedObject(const QObject *object) const;

public:
    QPointer<QQuickItem> m_target;

    QQmlComponent *m_topLeftHandleDelegate = nullptr;
    QQmlComponent *m_bottomRightHandleDelegate = nullptr;
    QPointer<QQuickItem> m_topLeftHandle;
    QPointer<QQuickItem> m_bottomRightHandle;
    QPointer<QQuickItem> m_draggedHandle = nullptr;

    QQuickSelectable *m_selectable = nullptr;

    QQuickTapHandler *m_tapHandler = nullptr;
    QQuickDragHandler *m_dragHandler = nullptr;

    QTimer m_scrollTimer;
    QPointF m_scrollToPoint;
    QSizeF m_scrollSpeed = QSizeF(1, 1);

    QQuickSelectionRectangle::SelectionMode m_selectionMode = QQuickSelectionRectangle::Auto;
    bool m_alwaysAcceptPressAndHold = false;

    bool m_enabled = true;
    bool m_active = false;
    bool m_dragging = false;
};

QT_END_NAMESPACE

#endif // QQUICKSELECTIONRECTANGLE_P_P_H
