/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QQUICKDRAGHANDLER_H
#define QQUICKDRAGHANDLER_H

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

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickDragAxis : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    QQuickDragAxis();

    qreal minimum() const { return m_minimum; }
    void setMinimum(qreal minimum);

    qreal maximum() const { return m_maximum; }
    void setMaximum(qreal maximum);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

signals:
    void minimumChanged();
    void maximumChanged();
    void enabledChanged();

private:
    qreal m_minimum;
    qreal m_maximum;
    bool m_enabled;
};

class Q_AUTOTEST_EXPORT QQuickDragHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(QQuickDragAxis * xAxis READ xAxis CONSTANT)
    Q_PROPERTY(QQuickDragAxis * yAxis READ yAxis CONSTANT)
    Q_PROPERTY(QVector2D translation READ translation NOTIFY translationChanged)

public:
    explicit QQuickDragHandler(QObject *parent = nullptr);
    ~QQuickDragHandler();

    void handleEventPoint(QQuickEventPoint *point) override;

    QQuickDragAxis *xAxis() { return &m_xAxis; }
    QQuickDragAxis *yAxis() { return &m_yAxis; }

    QVector2D translation() const { return m_translation; }
    void setTranslation(const QVector2D &trans);

    Q_INVOKABLE void enforceConstraints();

Q_SIGNALS:
//    void gestureStarted(QQuickGestureEvent *gesture);
    void translationChanged();

protected:
    bool wantsEventPoint(QQuickEventPoint *point) override;
    void onActiveChanged() override;
    void onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point) override;

private:
    void ungrab();
    void enforceAxisConstraints(QPointF *localPos);
    bool targetContains(QQuickEventPoint *point);
    QPointF localTargetPosition(QQuickEventPoint *point);

private:
    QPointF m_pressScenePos;
    QPointF m_pressTargetPos;   // We must also store the local targetPos, because we cannot deduce
                                // the press target pos from the scene pos in case there was e.g a
                                // flick in one of the ancestors during the drag.
    QVector2D m_translation;

    QQuickDragAxis m_xAxis;
    QQuickDragAxis m_yAxis;
    bool m_pressedInsideTarget = false;

    friend class QQuickDragAxis;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickDragHandler)
QML_DECLARE_TYPE(QQuickDragAxis)

#endif // QQUICKDRAGHANDLER_H
