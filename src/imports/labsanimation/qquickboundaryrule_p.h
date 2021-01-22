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

#ifndef QQUICKBOUNDARYRULE_H
#define QQUICKBOUNDARYRULE_H

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

#include <private/qtquickglobal_p.h>

#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickBoundaryRulePrivate;
class QQuickBoundaryRule : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickBoundaryRule)

    Q_INTERFACES(QQmlPropertyValueInterceptor)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(qreal minimumOvershoot READ minimumOvershoot WRITE setMinimumOvershoot NOTIFY minimumOvershootChanged)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(qreal maximumOvershoot READ maximumOvershoot WRITE setMaximumOvershoot NOTIFY maximumOvershootChanged)
    Q_PROPERTY(qreal overshootScale READ overshootScale WRITE setOvershootScale NOTIFY overshootScaleChanged)
    Q_PROPERTY(qreal currentOvershoot READ currentOvershoot NOTIFY currentOvershootChanged)
    Q_PROPERTY(qreal peakOvershoot READ peakOvershoot NOTIFY peakOvershootChanged)
    Q_PROPERTY(OvershootFilter overshootFilter READ overshootFilter WRITE setOvershootFilter NOTIFY overshootFilterChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)
    Q_PROPERTY(int returnDuration READ returnDuration WRITE setReturnDuration NOTIFY returnDurationChanged)
    QML_NAMED_ELEMENT(BoundaryRule)

public:
    enum OvershootFilter {
        None,
        Peak
    };
    Q_ENUM(OvershootFilter)

    QQuickBoundaryRule(QObject *parent=nullptr);
    ~QQuickBoundaryRule();

    void setTarget(const QQmlProperty &) override;
    void write(const QVariant &value) override;

    bool enabled() const;
    void setEnabled(bool enabled);

    qreal minimum() const;
    void setMinimum(qreal minimum);
    qreal minimumOvershoot() const;
    void setMinimumOvershoot(qreal minimum);

    qreal maximum() const;
    void setMaximum(qreal maximum);
    qreal maximumOvershoot() const;
    void setMaximumOvershoot(qreal maximum);

    qreal overshootScale() const;
    void setOvershootScale(qreal scale);

    qreal currentOvershoot() const;
    qreal peakOvershoot() const;

    OvershootFilter overshootFilter() const;
    void setOvershootFilter(OvershootFilter overshootFilter);

    Q_INVOKABLE bool returnToBounds();

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &easing);

    int returnDuration() const;
    void setReturnDuration(int duration);

Q_SIGNALS:
    void enabledChanged();
    void minimumChanged();
    void minimumOvershootChanged();
    void maximumChanged();
    void maximumOvershootChanged();
    void overshootScaleChanged();
    void currentOvershootChanged();
    void peakOvershootChanged();
    void overshootFilterChanged();
    void easingChanged();
    void returnDurationChanged();

private Q_SLOTS:
    void componentFinalized();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickBoundaryRule)

#endif // QQUICKBOUNDARYRULE_H
