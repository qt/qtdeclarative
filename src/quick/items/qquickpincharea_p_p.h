/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSG module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKPINCHAREA_P_H
#define QQUICKPINCHAREA_P_H

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

#include <qevent.h>

#include "qquickitem_p.h"
#include "qquickpincharea_p.h"

QT_BEGIN_NAMESPACE

class QQuickPinch;
class QQuickPinchAreaPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickPinchArea)
public:
    QQuickPinchAreaPrivate()
      : enabled(true), inPinch(false), pinchRejected(false), pinchActivated(false), initPinch(false)
    {
    }

    ~QQuickPinchAreaPrivate();

    void init()
    {
        Q_Q(QQuickPinchArea);
        q->setAcceptTouchEvents(true);
        q->setFiltersChildMouseEvents(true);
    }

    bool enabled : 1;
    bool inPinch : 1;
    bool pinchRejected : 1;
    bool pinchActivated : 1;
    bool initPinch : 1;
    int id1 = -1;
    QQuickPinch *pinch = nullptr;
    QPointF sceneStartPoint1;
    QPointF sceneStartPoint2;
    QPointF lastPoint1;
    QPointF lastPoint2;
    qreal pinchStartDist = 0;
    qreal pinchStartScale = 1;
    qreal pinchLastScale = 1;
    qreal pinchStartRotation = 0;
    qreal pinchStartAngle = 0;
    qreal pinchLastAngle = 0;
    qreal pinchRotation = 0;
    QPointF sceneStartCenter;
    QPointF pinchStartCenter;
    QPointF sceneLastCenter;
    QPointF pinchStartPos;
    QList<QEventPoint> touchPoints;
};

QT_END_NAMESPACE

#endif // QQUICKPINCHAREA_P_H

