// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquicktranslate_p.h"
#include "qquickitem_p.h"

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
    \nativetype QQuickTranslate
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-transforms
    \brief Provides a way to move an Item without changing its x or y properties.

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

/*!
    \qmlproperty real QtQuick::Translate::x

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
    \qmlproperty real QtQuick::Translate::y

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
    \nativetype QQuickScale
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-transforms
    \brief Provides a way to scale an Item.

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

/*!
    \qmlpropertygroup QtQuick::Scale::origin
    \qmlproperty real QtQuick::Scale::origin.x
    \qmlproperty real QtQuick::Scale::origin.y

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
    \qmlproperty real QtQuick::Scale::xScale

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
    \qmlproperty real QtQuick::Scale::yScale

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

/*!
    \qmlproperty real QtQuick::Scale::zScale
    \internal

    The scaling factor for the Z axis.

    The default value is 1.0.
*/
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
        : origin(), axis(0, 0, 1), angle(0), distanceToPlane(1024.0) {}
    QVector3D origin;
    QVector3D axis;
    qreal angle;
    qreal distanceToPlane;
};

/*!
    \qmltype Rotation
    \nativetype QQuickRotation
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-transforms
    \brief Provides a way to rotate an Item.

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

    \sa {customitems/dialcontrol}{Dial Control example}, {Qt Quick Demo - Clocks}
*/
QQuickRotation::QQuickRotation(QObject *parent)
    : QQuickTransform(*new QQuickRotationPrivate, parent)
{
}

/*!
    \qmlpropertygroup QtQuick::Rotation::origin
    \qmlproperty real QtQuick::Rotation::origin.x
    \qmlproperty real QtQuick::Rotation::origin.y

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
    \qmlproperty real QtQuick::Rotation::angle

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
    \qmlpropertygroup QtQuick::Rotation::axis
    \qmlproperty real QtQuick::Rotation::axis.x
    \qmlproperty real QtQuick::Rotation::axis.y
    \qmlproperty real QtQuick::Rotation::axis.z

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

/*!
    \qmlproperty real QtQuick::Rotation::distanceToPlane
    \since 6.11

    This property defines the distance between the view plane (the virtual screen)
    and the observer in the perspective projection model.

    A smaller distance produces a stronger perspective effect, making the object
    appear to recede or advance more dramatically during rotation. A larger value
    results in a flatter, more orthographic appearance with less visible
    perspective distortion.

    The default value is \c 1024.0, which provides a moderate perspective suitable
    for most use cases.

    When this property is set to \c 0, no perspective projection is applied.
    In this case, the rotation is performed directly in 3D space using the
    transformation defined by \l QMatrix4x4::rotate().

    This property only affects rotations around the x and y axes. Rotations around
    the z axis (2D rotations) are not influenced by this property.
*/
qreal QQuickRotation::distanceToPlane() const
{
    Q_D(const QQuickRotation);
    return d->distanceToPlane;
}

void QQuickRotation::setDistanceToPlane(qreal newDistanceToPlane)
{
    Q_D(QQuickRotation);
    if (qFuzzyCompare(d->distanceToPlane, newDistanceToPlane))
        return;
    d->distanceToPlane = newDistanceToPlane;
    emit distanceToPlaneChanged();
}

void QQuickRotation::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QQuickRotation);

    if (d->angle == 0. || d->axis.isNull())
        return;

    matrix->translate(d->origin);
    matrix->projectedRotate(d->angle, d->axis.x(), d->axis.y(), d->axis.z(), d->distanceToPlane);
    matrix->translate(-d->origin);
}

/*!
    \qmltype Shear
    \nativetype QQuickShear
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-transforms
    \since 6.9
    \brief Provides a way to shear an Item.

    The Shear type provides a way to transform an \l Item by a two-dimensional shear-type
    matrix, sometimes known as a \e skew transform.

    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Shear {
            xFactor: 1.0
        }
    }
    \endqml

    This shears the item by a factor of \c 1.0 along the x-axis without modifying anything along the
    y-axis. Each point \c P is displaced by \c{xFactor(P.y - origin.y)} (the signed vertical
    distance to the \l{origin} multiplied with the \l{xFactor}). Setting the \l{yFactor} shears the
    item along the y-axis and proportionally to the horizontal distance.

    \image x-shear.png

    Since the default origin is at \c{(0, 0)}, the top of the item remains untransformed, whereas
    the bottom is displaced 100 pixels to the right (corresponding to the height of the item.)

    This code is equivalent to the following:

    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Shear {
            xAngle: 45.0
        }
    }
    \endqml

    \note If both \c{xFactor}/\c{yFactor} and \c{xAngle}/\c{yAngle} are set, then the sum of the
    two displacements will be used.
*/
class QQuickShearPrivate : public QQuickTransformPrivate
{
public:
    QVector3D origin;
    qreal xFactor = 0.0;
    qreal yFactor = 0.0;
    qreal xAngle = 0.0;
    qreal yAngle = 0.0;
};

QQuickShear::QQuickShear(QObject *parent)
    : QQuickTransform(*new QQuickShearPrivate, parent)
{
}

/*!
    \qmlpropertygroup QtQuick::Shear::origin
    \qmlproperty real QtQuick::Shear::origin.x
    \qmlproperty real QtQuick::Shear::origin.y

    The origin point of the transformation (i.e., the point that stays fixed relative to the parent
    as the rest of the item is sheared).

    By default the origin is \c (0, 0).
*/
QVector3D QQuickShear::origin() const
{
    Q_D(const QQuickShear);
    return d->origin;
}

void QQuickShear::setOrigin(const QVector3D &point)
{
    Q_D(QQuickShear);
    if (d->origin == point)
        return;
    d->origin = point;
    update();
    emit originChanged();
}

/*!
    \qmlproperty real QtQuick::Shear::xFactor

    The factor by which to shear the item's coordinate system along the x-axis. Each point \c P is
    displaced by \c{xFactor(P.y - origin.y)}

    This corresponds to the \c sh parameter in \l{QTransform::shear()} and the \c xShear parameter
    in calls to \l{PlanarTransform::fromShear()}.

    The default value is \c 0.0.

    \sa xAngle
*/
qreal QQuickShear::xFactor() const
{
    Q_D(const QQuickShear);
    return d->xFactor;
}
void QQuickShear::setXFactor(qreal xFactor)
{
    Q_D(QQuickShear);
    if (d->xFactor == xFactor)
        return;
    d->xFactor = xFactor;
    update();
    emit xFactorChanged();
}

/*!
    \qmlproperty real QtQuick::Shear::yFactor

    The factor by which to shear the item's coordinate system along the y-axis. The factor by which
    to shear the item's coordinate system along the x-axis. Each point \c P is displaced by
    \c{xFactor(P.y - origin.y)}

    This corresponds to the \c sv parameter in \l{QTransform::shear()} and the \c yShear parameter
    in calls to \l{PlanarTransform::fromShear()}.

    The default value is \c 0.0.

    \sa yAngle
*/
qreal QQuickShear::yFactor() const
{
    Q_D(const QQuickShear);
    return d->yFactor;
}
void QQuickShear::setYFactor(qreal yFactor)
{
    Q_D(QQuickShear);
    if (d->yFactor == yFactor)
        return;
    d->yFactor = yFactor;
    update();
    emit yFactorChanged();
}

/*!
    \qmlproperty real QtQuick::Shear::xAngle

    The angle (in degrees) by which to shear the item's coordinate system along the x-axis. This
    is equivalent to setting \l{xFactor} to \c{tan(xAngle)}.

    The default value is \c 0.0.

    \sa xFactor
*/
qreal QQuickShear::xAngle() const
{
    Q_D(const QQuickShear);
    return d->xAngle;
}
void QQuickShear::setXAngle(qreal xAngle)
{
    Q_D(QQuickShear);
    if (d->xAngle == xAngle)
        return;
    d->xAngle = xAngle;
    update();
    emit xAngleChanged();
}

/*!
    \qmlproperty real QtQuick::Shear::yAngle

    The angle (in degrees) by which to shear the item's coordinate system along the y-axis. This
    is equivalent to setting \l{yFactor} to \c{tan(yAngle)}.

    The default value is \c 0.0.

    \sa yFactor
*/
qreal QQuickShear::yAngle() const
{
    Q_D(const QQuickShear);
    return d->yAngle;
}
void QQuickShear::setYAngle(qreal yAngle)
{
    Q_D(QQuickShear);
    if (d->yAngle == yAngle)
        return;
    d->yAngle = yAngle;
    update();
    emit yAngleChanged();
}

void QQuickShear::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QQuickShear);
    if (d->xFactor == 0.0 && d->yFactor == 0.0 && d->xAngle == 0.0 && d->yAngle == 0.0)
        return;

    const qreal xShear = qTan(qDegreesToRadians(d->xAngle)) + d->xFactor;
    const qreal yShear = qTan(qDegreesToRadians(d->yAngle)) + d->yFactor;

    matrix->translate(d->origin);
    *matrix *= QMatrix4x4(1.0, xShear, 0.0, 0.0,
                          yShear, 1.0, 0.0, 0.0,
                          0.0, 0.0, 1.0, 0.0,
                          0.0, 0.0, 0.0, 1.0);
    matrix->translate(-d->origin);
}


class QQuickMatrix4x4Private : public QQuickTransformPrivate
{
public:
    QQuickMatrix4x4Private()
        : matrix() {}
    QMatrix4x4 matrix;
};

/*!
    \qmltype Matrix4x4
    \nativetype QQuickMatrix4x4
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-transforms
    \since 5.3
    \brief Provides a way to apply a 4x4 tranformation matrix to an \l Item.

    The Matrix4x4 type provides a way to apply a transformation to an
    \l Item through a 4x4 matrix.

    It allows for a combination of rotation, scale, translatation and shearing
    by using just one tranformation provided in a 4x4-matrix.

    The following example rotates a Rectangle 45 degress (PI/4):

    \qml
    Rectangle {
        width: 100
        height: 100
        color: "red"

        transform: Matrix4x4 {
            property real a: Math.PI / 4
            matrix: Qt.matrix4x4(Math.cos(a), -Math.sin(a), 0, 0,
                                 Math.sin(a),  Math.cos(a), 0, 0,
                                 0,           0,            1, 0,
                                 0,           0,            0, 1)
        }
    }
    \endqml
*/
QQuickMatrix4x4::QQuickMatrix4x4(QObject *parent)
    : QQuickTransform(*new QQuickMatrix4x4Private, parent)
{
}

/*!
    \qmlproperty matrix4x4 QtQuick::Matrix4x4::matrix

    4x4-matrix which will be used in the tranformation of an \l Item
*/
QMatrix4x4 QQuickMatrix4x4::matrix() const
{
    Q_D(const QQuickMatrix4x4);
    return d->matrix;
}

void QQuickMatrix4x4::setMatrix(const QMatrix4x4 &matrix)
{
    Q_D(QQuickMatrix4x4);
    if (d->matrix == matrix)
         return;
    d->matrix = matrix;
    update();
    emit matrixChanged();
}

void QQuickMatrix4x4::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QQuickMatrix4x4);
    *matrix *= d->matrix;
}

QT_END_NAMESPACE

#include "moc_qquicktranslate_p.cpp"
