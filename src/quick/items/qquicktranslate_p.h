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

#ifndef QQUICKTRANSLATE_P_H
#define QQUICKTRANSLATE_P_H

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

#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

class QQuickTranslatePrivate;
class Q_AUTOTEST_EXPORT QQuickTranslate : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    QML_NAMED_ELEMENT(Translate)

public:
    QQuickTranslate(QObject *parent = nullptr);
    ~QQuickTranslate();

    qreal x() const;
    void setX(qreal);

    qreal y() const;
    void setY(qreal);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void xChanged();
    void yChanged();

private:
    Q_DECLARE_PRIVATE(QQuickTranslate)
    Q_DISABLE_COPY(QQuickTranslate)
};

class QQuickScalePrivate;
class Q_AUTOTEST_EXPORT QQuickScale : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(QVector3D origin READ origin WRITE setOrigin NOTIFY originChanged)
    Q_PROPERTY(qreal xScale READ xScale WRITE setXScale NOTIFY xScaleChanged)
    Q_PROPERTY(qreal yScale READ yScale WRITE setYScale NOTIFY yScaleChanged)
    Q_PROPERTY(qreal zScale READ zScale WRITE setZScale NOTIFY zScaleChanged)
    QML_NAMED_ELEMENT(Scale)
public:
    QQuickScale(QObject *parent = nullptr);
    ~QQuickScale();

    QVector3D origin() const;
    void setOrigin(const QVector3D &point);

    qreal xScale() const;
    void setXScale(qreal);

    qreal yScale() const;
    void setYScale(qreal);

    qreal zScale() const;
    void setZScale(qreal);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void originChanged();
    void xScaleChanged();
    void yScaleChanged();
    void zScaleChanged();
    void scaleChanged();

private:
    Q_DECLARE_PRIVATE(QQuickScale)
};

class QQuickRotationPrivate;
class Q_AUTOTEST_EXPORT QQuickRotation : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(QVector3D origin READ origin WRITE setOrigin NOTIFY originChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(QVector3D axis READ axis WRITE setAxis NOTIFY axisChanged)
    QML_NAMED_ELEMENT(Rotation)
public:
    QQuickRotation(QObject *parent = nullptr);
    ~QQuickRotation();

    QVector3D origin() const;
    void setOrigin(const QVector3D &point);

    qreal angle() const;
    void setAngle(qreal);

    QVector3D axis() const;
    void setAxis(const QVector3D &axis);
    void setAxis(Qt::Axis axis);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void originChanged();
    void angleChanged();
    void axisChanged();

private:
    Q_DECLARE_PRIVATE(QQuickRotation)
};

class QQuickMatrix4x4Private;
class Q_AUTOTEST_EXPORT QQuickMatrix4x4 : public QQuickTransform
{
    Q_OBJECT

    Q_PROPERTY(QMatrix4x4 matrix READ matrix WRITE setMatrix NOTIFY matrixChanged)
    QML_NAMED_ELEMENT(Matrix4x4)
    QML_ADDED_IN_MINOR_VERSION(3)
public:
    QQuickMatrix4x4(QObject *parent = nullptr);
    ~QQuickMatrix4x4();

    QMatrix4x4 matrix() const;
    void setMatrix(const QMatrix4x4& matrix);

    void applyTo(QMatrix4x4 *matrix) const override;

Q_SIGNALS:
    void matrixChanged();

private:
    Q_DECLARE_PRIVATE(QQuickMatrix4x4)
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTranslate)

#endif
