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

#ifndef QQUICKPINCHHANDLER_H
#define QQUICKPINCHHANDLER_H

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
#include "qquickmultipointhandler_p.h"
#include <private/qquicktranslate_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickPinchHandler : public QQuickMultiPointHandler
{
    Q_OBJECT
    Q_PROPERTY(qreal minimumScale READ minimumScale WRITE setMinimumScale NOTIFY minimumScaleChanged)
    Q_PROPERTY(qreal maximumScale READ maximumScale WRITE setMaximumScale NOTIFY maximumScaleChanged)
    Q_PROPERTY(qreal minimumRotation READ minimumRotation WRITE setMinimumRotation NOTIFY minimumRotationChanged)
    Q_PROPERTY(qreal maximumRotation READ maximumRotation WRITE setMaximumRotation NOTIFY maximumRotationChanged)
    Q_PROPERTY(PinchOrigin pinchOrigin READ pinchOrigin WRITE setPinchOrigin NOTIFY pinchOriginChanged)
    Q_PROPERTY(QPointF centroid READ centroid NOTIFY updated)
    Q_PROPERTY(QVector2D centroidVelocity READ centroidVelocity NOTIFY updated)
    Q_PROPERTY(qreal scale READ scale NOTIFY updated)
    Q_PROPERTY(qreal rotation READ rotation NOTIFY updated)
    Q_PROPERTY(QVector2D translation READ translation NOTIFY updated)
    Q_PROPERTY(qreal minimumX READ minimumX WRITE setMinimumX NOTIFY minimumXChanged)
    Q_PROPERTY(qreal maximumX READ maximumX WRITE setMaximumX NOTIFY maximumXChanged)
    Q_PROPERTY(qreal minimumY READ minimumY WRITE setMinimumY NOTIFY minimumYChanged)
    Q_PROPERTY(qreal maximumY READ maximumY WRITE setMaximumY NOTIFY maximumYChanged)

public:
    enum PinchOrigin {
        FirstPoint, PinchCenter, TargetCenter
    };
    Q_ENUM(PinchOrigin)

    explicit QQuickPinchHandler(QObject *parent = nullptr);
    ~QQuickPinchHandler();

    qreal minimumScale() const { return m_minimumScale; }
    void setMinimumScale(qreal minimumScale);

    qreal maximumScale() const { return m_maximumScale; }
    void setMaximumScale(qreal maximumScale);

    qreal minimumRotation() const { return m_minimumRotation; }
    void setMinimumRotation(qreal minimumRotation);

    qreal maximumRotation() const { return m_maximumRotation; }
    void setMaximumRotation(qreal maximumRotation);

    PinchOrigin pinchOrigin() const { return m_pinchOrigin; }
    void setPinchOrigin(PinchOrigin pinchOrigin);

    QVector2D translation() const { return m_activeTranslation; }
    qreal scale() const { return m_activeScale; }
    qreal rotation() const { return m_activeRotation; }
    QPointF centroid() const { return m_centroid; }
    QVector2D centroidVelocity() const { return m_centroidVelocity; }

    qreal minimumX() const { return m_minimumX; }
    void setMinimumX(qreal minX);
    qreal maximumX() const { return m_maximumX; }
    void setMaximumX(qreal maxX);
    qreal minimumY() const { return m_minimumY; }
    void setMinimumY(qreal minY);
    qreal maximumY() const { return m_maximumY; }
    void setMaximumY(qreal maxY);

signals:
    void minimumScaleChanged();
    void maximumScaleChanged();
    void minimumRotationChanged();
    void maximumRotationChanged();
    void minimumXChanged();
    void maximumXChanged();
    void minimumYChanged();
    void maximumYChanged();
    void pinchOriginChanged();
    void updated();

protected:
    bool wantsPointerEvent(QQuickPointerEvent *event) override;
    void onActiveChanged() override;
    void handlePointerEventImpl(QQuickPointerEvent *event) override;

private:
    // properties
    qreal m_activeScale;
    qreal m_activeRotation;
    QVector2D m_activeTranslation;
    QPointF m_centroid;
    QVector2D m_centroidVelocity;

    qreal m_minimumScale;
    qreal m_maximumScale;

    qreal m_minimumRotation;
    qreal m_maximumRotation;

    qreal m_minimumX;
    qreal m_maximumX;
    qreal m_minimumY;
    qreal m_maximumY;

    PinchOrigin m_pinchOrigin;

    // internal
    qreal m_startScale;
    qreal m_startRotation;
    QPointF m_startCentroid;
    qreal m_startDistance;
    QPointF m_startPos;

    QVector<PointData> m_startAngles;
    QMatrix4x4 m_startMatrix;
    QQuickMatrix4x4 m_transform;

};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPinchHandler)

#endif // QQUICKPINCHHANDLER_H
