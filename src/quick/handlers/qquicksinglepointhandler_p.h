/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickSinglePointHandler;

class Q_QUICK_PRIVATE_EXPORT QQuickHandlerPoint {
    Q_GADGET
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(QPointingDeviceUniqueId uniqueId READ uniqueId)
    Q_PROPERTY(QPointF position READ position)
    Q_PROPERTY(QPointF scenePosition READ scenePosition)
    Q_PROPERTY(QPointF pressPosition READ pressPosition)
    Q_PROPERTY(QPointF scenePressPosition READ scenePressPosition)
    Q_PROPERTY(QPointF sceneGrabPosition READ sceneGrabPosition)
    Q_PROPERTY(Qt::MouseButtons pressedButtons READ pressedButtons)
    Q_PROPERTY(QVector2D velocity READ velocity)
    Q_PROPERTY(qreal rotation READ rotation)
    Q_PROPERTY(qreal pressure READ pressure)
    Q_PROPERTY(QSizeF ellipseDiameters READ ellipseDiameters)

public:
    QQuickHandlerPoint();

    int id() const { return m_id; }
    Qt::MouseButtons pressedButtons() const { return m_pressedButtons; }
    QPointF pressPosition() const { return m_pressPosition; }
    QPointF scenePressPosition() const { return m_scenePressPosition; }
    QPointF sceneGrabPosition() const { return m_sceneGrabPosition; }
    QPointF position() const { return m_position; }
    QPointF scenePosition() const { return m_scenePosition; }
    QVector2D velocity() const { return m_velocity; }
    qreal rotation() const { return m_rotation; }
    qreal pressure() const { return m_pressure; }
    QSizeF ellipseDiameters() const { return m_ellipseDiameters; }
    QPointingDeviceUniqueId uniqueId() const { return m_uniqueId; }

private:
    void reset();
    int m_id;
    QPointingDeviceUniqueId m_uniqueId;
    Qt::MouseButtons m_pressedButtons;
    QPointF m_position;
    QPointF m_scenePosition;
    QPointF m_pressPosition;
    QPointF m_scenePressPosition;
    QPointF m_sceneGrabPosition;
    QVector2D m_velocity;
    qreal m_rotation;
    qreal m_pressure;
    QSizeF m_ellipseDiameters;
    friend class QQuickSinglePointHandler;
};

class Q_QUICK_PRIVATE_EXPORT QQuickSinglePointHandler : public QQuickPointerDeviceHandler
{
    Q_OBJECT
    Q_PROPERTY(Qt::MouseButtons acceptedButtons READ acceptedButtons WRITE setAcceptedButtons NOTIFY acceptedButtonsChanged)
    Q_PROPERTY(QQuickHandlerPoint point READ point NOTIFY pointChanged)
public:
    explicit QQuickSinglePointHandler(QObject *parent = nullptr);
    virtual ~QQuickSinglePointHandler() { }

    Qt::MouseButtons acceptedButtons() const { return m_acceptedButtons; }
    void setAcceptedButtons(Qt::MouseButtons buttons);

    QQuickHandlerPoint point() const { return m_pointInfo; }

Q_SIGNALS:
    void pointChanged();
    void singlePointGrabChanged(); // QQuickPointerHandler::grabChanged signal can't be a property notifier here
    void acceptedButtonsChanged();

protected:
    bool wantsPointerEvent(QQuickPointerEvent *event) override;
    virtual bool wantsEventPoint(QQuickEventPoint *point);
    void handlePointerEventImpl(QQuickPointerEvent *event) override;
    virtual void handleEventPoint(QQuickEventPoint *point) = 0;

    QQuickEventPoint *currentPoint(QQuickPointerEvent *ev) { return ev->pointById(m_pointInfo.m_id); }
    void onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point) override;

    void setIgnoreAdditionalPoints(bool v = true);

    void moveTarget(QPointF pos, QQuickEventPoint *point);

private:
    void reset();

private:
    QQuickHandlerPoint m_pointInfo;
    Qt::MouseButtons m_acceptedButtons;
    bool m_ignoreAdditionalPoints : 1;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickHandlerPoint)
QML_DECLARE_TYPE(QQuickSinglePointHandler)

#endif // QQUICKPOINTERSINGLEHANDLER_H
