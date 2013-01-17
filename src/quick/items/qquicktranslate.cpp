/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktranslate_p.h"
#include "qquickitem_p.h"

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

class QQuickTranslatePrivate : public QQuickTransformPrivate
{
public:
    QQuickTranslatePrivate()
    : x(0), y(0) {}

    qreal x;
    qreal y;
};


/*!
    \qmltype Translate
    \instantiates QQuickTranslate
    \inqmlmodule QtQuick 2
    \ingroup qtquick-visual-transforms
    \brief Provides a way to move an Item without changing its x or y properties

    The Translate type provides independent control over position in addition
    to the Item's x and y properties.

    The following example moves the Y axis of the \l Rectangle items while
    still allowing the \l Row to lay the items out as if they had not been
    transformed:

    \qml
    import QtQuick 2.0

    Row {
        Rectangle {
            width: 100; height: 100
            color: "blue"
            transform: Translate { y: 20 }
        }
        Rectangle {
            width: 100; height: 100
            color: "red"
            transform: Translate { y: -20 }
        }
    }
    \endqml

    \image translate.png
*/
QQuickTranslate::QQuickTranslate(QObject *parent)
: QQuickTransform(*new QQuickTranslatePrivate, parent)
{
}


QQuickTranslate::~QQuickTranslate()
{
}
/*!
    \qmlproperty real QtQuick2::Translate::x

    The translation along the X axis.

    The default value is 0.0.
*/
qreal QQuickTranslate::x() const
{
    Q_D(const QQuickTranslate);
    return d->x;
}

void QQuickTranslate::setX(qreal x)
{
    Q_D(QQuickTranslate);
    if (d->x == x)
        return;
    d->x = x;
    update();
    emit xChanged();
}

/*!
    \qmlproperty real QtQuick2::Translate::y

    The translation along the Y axis.

    The default value is 0.0.
*/
qreal QQuickTranslate::y() const
{
    Q_D(const QQuickTranslate);
    return d->y;
}
void QQuickTranslate::setY(qreal y)
{
    Q_D(QQuickTranslate);
    if (d->y == y)
        return;
    d->y = y;
    update();
    emit yChanged();
}

void QQuickTranslate::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QQuickTranslate);
    matrix->translate(d->x, d->y, 0);
}

class QQuickScalePrivate : public QQuickTransformPrivate
{
public:
    QQuickScalePrivate()
        : xScale(1), yScale(1), zScale(1) {}
    QVector3D origin;
    qreal xScale;
    qreal yScale;
    qreal zScale;
};

/*!
    \qmltype Scale
    \instantiates QQuickScale
    \inqmlmodule QtQuick 2
    \ingroup qtquick-visual-transforms
    \brief Provides a way to scale an Item

    The Scale type provides a way to scale an \l Item through a scale-type
    transform.

    It allows different scaling values for the x and y axes, and allows the
    scale to be relative to an arbitrary point. This gives more control over
    item scaling than the \l{Item::}{scale} property.

    The following example scales the X axis of the Rectangle, relative to
    its interior point (25, 25):

    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Scale { origin.x: 25; origin.y: 25; xScale: 3}
    }
    \endqml

    \sa Rotation, Translate
*/
QQuickScale::QQuickScale(QObject *parent)
    : QQuickTransform(*new QQuickScalePrivate, parent)
{
}

QQuickScale::~QQuickScale()
{
}

/*!
    \qmlproperty real QtQuick2::Scale::origin.x
    \qmlproperty real QtQuick2::Scale::origin.y

    This property holds the point that the item is scaled from (that is,
    the point that stays fixed relative to the parent as the rest of the
    item grows).

    The default value of the origin is (0, 0).
*/
QVector3D QQuickScale::origin() const
{
    Q_D(const QQuickScale);
    return d->origin;
}
void QQuickScale::setOrigin(const QVector3D &point)
{
    Q_D(QQuickScale);
    if (d->origin == point)
        return;
    d->origin = point;
    update();
    emit originChanged();
}

/*!
    \qmlproperty real QtQuick2::Scale::xScale

    The scaling factor for the X axis.

    The default value is 1.0.
*/
qreal QQuickScale::xScale() const
{
    Q_D(const QQuickScale);
    return d->xScale;
}
void QQuickScale::setXScale(qreal scale)
{
    Q_D(QQuickScale);
    if (d->xScale == scale)
        return;
    d->xScale = scale;
    update();
    emit xScaleChanged();
    emit scaleChanged();
}

/*!
    \qmlproperty real QtQuick2::Scale::yScale

    The scaling factor for the Y axis.

    The default value is 1.0.
*/
qreal QQuickScale::yScale() const
{
    Q_D(const QQuickScale);
    return d->yScale;
}
void QQuickScale::setYScale(qreal scale)
{
    Q_D(QQuickScale);
    if (d->yScale == scale)
        return;
    d->yScale = scale;
    update();
    emit yScaleChanged();
    emit scaleChanged();
}

qreal QQuickScale::zScale() const
{
    Q_D(const QQuickScale);
    return d->zScale;
}
void QQuickScale::setZScale(qreal scale)
{
    Q_D(QQuickScale);
    if (d->zScale == scale)
        return;
    d->zScale = scale;
    update();
    emit zScaleChanged();
    emit scaleChanged();
}

void QQuickScale::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QQuickScale);
    matrix->translate(d->origin);
    matrix->scale(d->xScale, d->yScale, d->zScale);
    matrix->translate(-d->origin);
}

class QQuickRotationPrivate : public QQuickTransformPrivate
{
public:
    QQuickRotationPrivate()
        : angle(0), axis(0, 0, 1) {}
    QVector3D origin;
    qreal angle;
    QVector3D axis;
};

/*!
    \qmltype Rotation
    \instantiates QQuickRotation
    \inqmlmodule QtQuick 2
    \ingroup qtquick-visual-transforms
    \brief Provides a way to rotate an Item

    The Rotation type provides a way to rotate an \l Item through a
    rotation-type transform.

    It allows (z axis) rotation to be relative to an arbitrary point, and also
    provides a way to specify 3D-like rotations for Items. This gives more
    control over item rotation than the \l{Item::}{rotation} property.

    The following example rotates a Rectangle around its interior point
    (25, 25):

    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Rotation { origin.x: 25; origin.y: 25; angle: 45}
    }
    \endqml

    For 3D-like item rotations, you must specify the axis of rotation in
    addition to the origin point. The following example shows various 3D-like
    rotations applied to an \l Image.

    \snippet qml/rotation.qml 0

    \image axisrotation.png

    \sa {declarative/ui-components/dialcontrol}{Dial Control example}, {declarative/toys/clocks}{Clocks example}
*/
QQuickRotation::QQuickRotation(QObject *parent)
    : QQuickTransform(*new QQuickRotationPrivate, parent)
{
}

QQuickRotation::~QQuickRotation()
{
}

/*!
    \qmlproperty real QtQuick2::Rotation::origin.x
    \qmlproperty real QtQuick2::Rotation::origin.y

    The origin point of the rotation (i.e., the point that stays fixed
    relative to the parent as the rest of the item rotates). By default
    the origin is (0, 0).
*/
QVector3D QQuickRotation::origin() const
{
    Q_D(const QQuickRotation);
    return d->origin;
}

void QQuickRotation::setOrigin(const QVector3D &point)
{
    Q_D(QQuickRotation);
    if (d->origin == point)
        return;
    d->origin = point;
    update();
    emit originChanged();
}

/*!
    \qmlproperty real QtQuick2::Rotation::angle

    The angle to rotate, in degrees clockwise.
*/
qreal QQuickRotation::angle() const
{
    Q_D(const QQuickRotation);
    return d->angle;
}
void QQuickRotation::setAngle(qreal angle)
{
    Q_D(QQuickRotation);
    if (d->angle == angle)
        return;
    d->angle = angle;
    update();
    emit angleChanged();
}

/*!
    \qmlproperty real QtQuick2::Rotation::axis.x
    \qmlproperty real QtQuick2::Rotation::axis.y
    \qmlproperty real QtQuick2::Rotation::axis.z

    The axis to rotate around. For simple (2D) rotation around a point, you
    do not need to specify an axis, as the default axis is the z axis
    (\c{ axis { x: 0; y: 0; z: 1 } }).

    For a typical 3D-like rotation you will usually specify both the origin
    and the axis.

    \image 3d-rotation-axis.png
*/
QVector3D QQuickRotation::axis() const
{
    Q_D(const QQuickRotation);
    return d->axis;
}
void QQuickRotation::setAxis(const QVector3D &axis)
{
    Q_D(QQuickRotation);
    if (d->axis == axis)
         return;
    d->axis = axis;
    update();
    emit axisChanged();
}

void QQuickRotation::setAxis(Qt::Axis axis)
{
    switch (axis)
    {
    case Qt::XAxis:
        setAxis(QVector3D(1, 0, 0));
        break;
    case Qt::YAxis:
        setAxis(QVector3D(0, 1, 0));
        break;
    case Qt::ZAxis:
        setAxis(QVector3D(0, 0, 1));
        break;
    }
}

class QGraphicsRotation {
public:
    static inline void projectedRotate(QMatrix4x4 *matrix, qreal angle, qreal x, qreal y, qreal z)
    {
        matrix->projectedRotate(angle, x, y, z);
    }
};

void QQuickRotation::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QQuickRotation);

    if (d->angle == 0. || d->axis.isNull())
        return;

    matrix->translate(d->origin);
    QGraphicsRotation::projectedRotate(matrix, d->angle, d->axis.x(), d->axis.y(), d->axis.z());
    matrix->translate(-d->origin);
}

QT_END_NAMESPACE
