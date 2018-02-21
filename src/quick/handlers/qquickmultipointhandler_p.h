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

#ifndef QQUICKPOINTERMULTIHANDLER_H
#define QQUICKPOINTERMULTIHANDLER_H

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

#include "qquickitem.h"
#include "qevent.h"
#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickMultiPointHandler : public QQuickPointerDeviceHandler
{
    Q_OBJECT
    Q_PROPERTY(int minimumPointCount READ minimumPointCount WRITE setMinimumPointCount NOTIFY minimumPointCountChanged)
    Q_PROPERTY(int maximumPointCount READ maximumPointCount WRITE setMaximumPointCount NOTIFY maximumPointCountChanged)
    Q_PROPERTY(qreal pointDistanceThreshold READ pointDistanceThreshold WRITE setPointDistanceThreshold NOTIFY pointDistanceThresholdChanged)

public:
    explicit QQuickMultiPointHandler(QObject *parent = nullptr, int minimumPointCount = 2);
    ~QQuickMultiPointHandler();

    int minimumPointCount() const { return m_minimumPointCount; }
    void setMinimumPointCount(int c);

    int maximumPointCount() const { return m_maximumPointCount >= 0 ? m_maximumPointCount : m_minimumPointCount; }
    void setMaximumPointCount(int maximumPointCount);

    qreal pointDistanceThreshold() const { return m_pointDistanceThreshold; }
    void setPointDistanceThreshold(qreal pointDistanceThreshold);

signals:
    void minimumPointCountChanged();
    void maximumPointCountChanged();
    void pointDistanceThresholdChanged();

protected:
    struct PointData {
        PointData() : id(0), angle(0) {}
        PointData(quint64 id, qreal angle) : id(id), angle(angle) {}
        quint64 id;
        qreal angle;
    };

    bool wantsPointerEvent(QQuickPointerEvent *event) override;
    bool sameAsCurrentPoints(QQuickPointerEvent *event);
    QVector<QQuickEventPoint *> eligiblePoints(QQuickPointerEvent *event);
    QPointF touchPointCentroid();
    QVector2D touchPointCentroidVelocity();
    qreal averageTouchPointDistance(const QPointF &ref);
    qreal averageStartingDistance(const QPointF &ref);
    qreal averageTouchPointAngle(const QPointF &ref);
    qreal averageStartingAngle(const QPointF &ref);
    QVector<PointData> angles(const QPointF &ref) const;
    static qreal averageAngleDelta(const QVector<PointData> &old, const QVector<PointData> &newAngles);
    void acceptPoints(const QVector<QQuickEventPoint *> &points);
    bool grabPoints(QVector<QQuickEventPoint *> points);

protected:
    QVector<QQuickEventPoint *> m_currentPoints;
    int m_minimumPointCount;
    int m_maximumPointCount;
    qreal m_pointDistanceThreshold;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMultiPointHandler)

#endif // QQUICKPOINTERMULTIHANDLER_H
