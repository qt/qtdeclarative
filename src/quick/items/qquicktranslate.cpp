/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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
    Constructs an empty QQuickTranslate object with the given \a parent.
*/
QQuickTranslate::QQuickTranslate(QObject *parent)
: QQuickTransform(*new QQuickTranslatePrivate, parent)
{
}

/*!
    Destroys the graphics scale.
*/
QQuickTranslate::~QQuickTranslate()
{
}

/*!
    \property QQuickTranslate::x
    \brief the horizontal translation.

    The translation can be any real number; the default value is 0.0.

    \sa y
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
    \property QQuickTranslate::y
    \brief the vertical translation.

    The translation can be any real number; the default value is 0.0.

    \sa x
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

QQuickScale::QQuickScale(QObject *parent)
    : QQuickTransform(*new QQuickScalePrivate, parent)
{
}

QQuickScale::~QQuickScale()
{
}

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

QQuickRotation::QQuickRotation(QObject *parent)
    : QQuickTransform(*new QQuickRotationPrivate, parent)
{
}

QQuickRotation::~QQuickRotation()
{
}

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
