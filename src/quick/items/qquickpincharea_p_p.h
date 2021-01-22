/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

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
      : enabled(true), stealMouse(false), inPinch(false)
      , pinchRejected(false), pinchActivated(false), initPinch(false)
      , pinch(0), pinchStartDist(0), pinchStartScale(1.0)
      , pinchLastScale(1.0), pinchStartRotation(0.0), pinchStartAngle(0.0)
      , pinchLastAngle(0.0), pinchRotation(0.0)
    {
    }

    ~QQuickPinchAreaPrivate();

    void init()
    {
        Q_Q(QQuickPinchArea);
        q->setAcceptedMouseButtons(Qt::LeftButton);
        q->setFiltersChildMouseEvents(true);
    }

    bool enabled : 1;
    bool stealMouse : 1;
    bool inPinch : 1;
    bool pinchRejected : 1;
    bool pinchActivated : 1;
    bool initPinch : 1;
    QQuickPinch *pinch;
    QPointF sceneStartPoint1;
    QPointF sceneStartPoint2;
    QPointF lastPoint1;
    QPointF lastPoint2;
    qreal pinchStartDist;
    qreal pinchStartScale;
    qreal pinchLastScale;
    qreal pinchStartRotation;
    qreal pinchStartAngle;
    qreal pinchLastAngle;
    qreal pinchRotation;
    QPointF sceneStartCenter;
    QPointF pinchStartCenter;
    QPointF sceneLastCenter;
    QPointF pinchStartPos;
    QList<QTouchEvent::TouchPoint> touchPoints;
    int id1;
};

QT_END_NAMESPACE

#endif // QQUICKPINCHAREA_P_H

