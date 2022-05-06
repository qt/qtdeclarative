/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QQUICKPOINTERSINGLEHANDLER_H
#define QQUICKPOINTERSINGLEHANDLER_H

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

#include "qquickhandlerpoint_p.h"
#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickSinglePointHandlerPrivate;

class Q_QUICK_PRIVATE_EXPORT QQuickSinglePointHandler : public QQuickPointerDeviceHandler
{
    Q_OBJECT
    Q_PROPERTY(QQuickHandlerPoint point READ point NOTIFY pointChanged)

public:
    explicit QQuickSinglePointHandler(QQuickItem *parent = nullptr);

    QQuickHandlerPoint point() const;

Q_SIGNALS:
    void pointChanged();

protected:
    QQuickSinglePointHandler(QQuickSinglePointHandlerPrivate &dd, QQuickItem *parent);

    bool wantsPointerEvent(QPointerEvent *event) override;
    void handlePointerEventImpl(QPointerEvent *event) override;
    virtual void handleEventPoint(QPointerEvent *event, QEventPoint &point);

    QEventPoint &currentPoint(QPointerEvent *ev);
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point) override;

    void setIgnoreAdditionalPoints(bool v = true);

    void moveTarget(QPointF pos, QEventPoint &point);

    void setPointId(int id);

    Q_DECLARE_PRIVATE(QQuickSinglePointHandler)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSinglePointHandler)

#endif // QQUICKPOINTERSINGLEHANDLER_H
