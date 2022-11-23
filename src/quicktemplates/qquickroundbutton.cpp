// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickroundbutton_p.h"

#include <QtQuickTemplates2/private/qquickbutton_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RoundButton
    \inherits Button
//!     \instantiates QQuickRoundButton
    \inqmlmodule QtQuick.Controls
    \since 5.8
    \ingroup qtquickcontrols-buttons
    \brief A push-button control with rounded corners that can be clicked by the user.

    \image qtquickcontrols-roundbutton.png

    RoundButton is identical to \l Button, except that it has a \l radius property
    which allows the corners to be rounded without having to customize the
    \l background.

    \snippet qtquickcontrols-roundbutton.qml 1

    \sa {Customizing RoundButton}, {Button Controls}
*/

class QQuickRoundButtonPrivate : public QQuickButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickRoundButton)

public:
    void setRadius(qreal newRadius = -1.0);

    qreal radius = 0;
    bool explicitRadius = false;
};

void QQuickRoundButtonPrivate::setRadius(qreal newRadius)
{
    Q_Q(QQuickRoundButton);
    const qreal oldRadius = radius;
    if (newRadius < 0)
        radius = qMax<qreal>(0, qMin<qreal>(width, height) / 2);
    else
        radius = newRadius;

    if (!qFuzzyCompare(radius, oldRadius))
        emit q->radiusChanged();
}

QQuickRoundButton::QQuickRoundButton(QQuickItem *parent)
    : QQuickButton(*(new QQuickRoundButtonPrivate), parent)
{
}

/*!
    \qmlproperty real QtQuick.Controls::RoundButton::radius

    This property holds the radius of the button.

    To create a relatively square button that has slightly rounded corners,
    use a small value, such as \c 3.

    To create a completely circular button (the default), use a value that is
    equal to half of the width or height of the button, and make the button's
    width and height identical.

    To reset this property back to the default value, set its value to
    \c undefined.
*/
qreal QQuickRoundButton::radius() const
{
    Q_D(const QQuickRoundButton);
    return d->radius;
}

void QQuickRoundButton::setRadius(qreal radius)
{
    Q_D(QQuickRoundButton);
    d->explicitRadius = true;
    d->setRadius(radius);
}

void QQuickRoundButton::resetRadius()
{
    Q_D(QQuickRoundButton);
    d->explicitRadius = false;
    d->setRadius();
}

void QQuickRoundButton::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickRoundButton);
    QQuickControl::geometryChange(newGeometry, oldGeometry);
    if (!d->explicitRadius)
        d->setRadius();
}

QT_END_NAMESPACE

#include "moc_qquickroundbutton_p.cpp"
