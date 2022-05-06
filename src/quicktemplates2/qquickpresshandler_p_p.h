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

#ifndef QQUICKPRESSHANDLER_P_P_H
#define QQUICKPRESSHANDLER_P_P_H

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

#include <QtCore/qpoint.h>
#include <QtCore/qbasictimer.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QMouseEvent;
class QTimerEvent;

struct QQuickPressHandler
{
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event);

    void clearDelayedMouseEvent();
    bool isActive();

    static bool isSignalConnected(QQuickItem *item, const char *signalName, int &signalIndex);

    QQuickItem *control = nullptr;
    QBasicTimer timer;
    QPointF pressPos;
    bool longPress = false;
    int pressAndHoldSignalIndex = -1;
    int pressedSignalIndex = -1;
    int releasedSignalIndex = -1;
    QMouseEvent *delayedMousePressEvent = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPRESSHANDLER_P_P_H
