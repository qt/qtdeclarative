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

#ifndef QQUICKPOINTERHANDLER_P_H
#define QQUICKPOINTERHANDLER_P_H

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

#include "qevent.h"

#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickPointerHandlerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPointerHandler)

public:
    static QQuickPointerHandlerPrivate* get(QQuickPointerHandler *q) { return q->d_func(); }
    static const QQuickPointerHandlerPrivate* get(const QQuickPointerHandler *q) { return q->d_func(); }

    QQuickPointerHandlerPrivate();

    template<typename TEventPoint>
    bool dragOverThreshold(qreal d, Qt::Axis axis, const TEventPoint *p) const;

    bool dragOverThreshold(QVector2D delta) const;
    bool dragOverThreshold(const QQuickEventPoint *point) const;

    QQuickPointerEvent *currentEvent = nullptr;
    QQuickItem *target = nullptr;
    qreal m_margin = 0;
    qint16 dragThreshold = -1;   // -1 means use the platform default
    uint8_t grabPermissions : 8;
    Qt::CursorShape cursorShape : 6;
    bool enabled : 1;
    bool active : 1;
    bool targetExplicitlySet : 1;
    bool hadKeepMouseGrab : 1;    // some handlers override target()->setKeepMouseGrab(); this remembers previous state
    bool hadKeepTouchGrab : 1;    // some handlers override target()->setKeepTouchGrab(); this remembers previous state
    bool cursorSet : 1;
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERHANDLER_P_H
