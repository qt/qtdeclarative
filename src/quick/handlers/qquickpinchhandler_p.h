// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include "qquickdragaxis_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickPinchHandler : public QQuickMultiPointHandler
{
    Q_OBJECT
    Q_PROPERTY(qreal minimumScale READ minimumScale WRITE setMinimumScale NOTIFY minimumScaleChanged)
    Q_PROPERTY(qreal maximumScale READ maximumScale WRITE setMaximumScale NOTIFY maximumScaleChanged)
    Q_PROPERTY(qreal minimumRotation READ minimumRotation WRITE setMinimumRotation NOTIFY minimumRotationChanged)
    Q_PROPERTY(qreal maximumRotation READ maximumRotation WRITE setMaximumRotation NOTIFY maximumRotationChanged)
    Q_PROPERTY(qreal scale READ scale NOTIFY updated)
    Q_PROPERTY(qreal activeScale READ activeScale NOTIFY updated)
    Q_PROPERTY(qreal rotation READ rotation NOTIFY updated)
    Q_PROPERTY(QVector2D translation READ translation NOTIFY updated)
    Q_PROPERTY(QQuickDragAxis * xAxis READ xAxis CONSTANT)
    Q_PROPERTY(QQuickDragAxis * yAxis READ yAxis CONSTANT)
    QML_NAMED_ELEMENT(PinchHandler)
    QML_ADDED_IN_VERSION(2, 12)

public:
    explicit QQuickPinchHandler(QQuickItem *parent = nullptr);

    qreal minimumScale() const { return m_minimumScale; }
    void setMinimumScale(qreal minimumScale);

    qreal maximumScale() const { return m_maximumScale; }
    void setMaximumScale(qreal maximumScale);

    qreal minimumRotation() const { return m_minimumRotation; }
    void setMinimumRotation(qreal minimumRotation);

    qreal maximumRotation() const { return m_maximumRotation; }
    void setMaximumRotation(qreal maximumRotation);

    QVector2D translation() const { return m_activeTranslation; }
    qreal scale() const { return m_accumulatedScale; }
    qreal activeScale() const { return m_activeScale; }
    qreal rotation() const { return m_activeRotation; }

    QQuickDragAxis *xAxis() { return &m_xAxis; }
    QQuickDragAxis *yAxis() { return &m_yAxis; }

Q_SIGNALS:
    void minimumScaleChanged();
    void maximumScaleChanged();
    void minimumRotationChanged();
    void maximumRotationChanged();
    void updated();

protected:
    bool wantsPointerEvent(QPointerEvent *event) override;
    void onActiveChanged() override;
    void handlePointerEventImpl(QPointerEvent *event) override;

private:
    // properties
    qreal m_activeScale = 1;
    qreal m_accumulatedScale = 1;
    qreal m_activeRotation = 0;
    QVector2D m_activeTranslation = QVector2D(0, 0);

    qreal m_minimumScale = -qInf();
    qreal m_maximumScale = qInf();

    qreal m_minimumRotation = -qInf();
    qreal m_maximumRotation = qInf();

    QQuickDragAxis m_xAxis;
    QQuickDragAxis m_yAxis;

    // internal
    qreal m_startScale = 1;
    qreal m_startRotation = 0;
    qreal m_startDistance = 0;
    QPointF m_startPos;
    qreal m_accumulatedStartCentroidDistance = 0;
    QVector<PointData> m_startAngles;
    QQuickMatrix4x4 m_transform;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPinchHandler)

#endif // QQUICKPINCHHANDLER_H
