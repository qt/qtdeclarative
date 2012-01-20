// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef QQUICKRECTANGLE_P_H
#define QQUICKRECTANGLE_P_H

#include "qquickitem.h"

#include <QtGui/qbrush.h>

#include <private/qtquickglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickPen : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY penChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY penChanged)
    Q_PROPERTY(bool aligned READ aligned WRITE setAligned NOTIFY penChanged)
public:
    QQuickPen(QObject *parent=0);

    qreal width() const;
    void setWidth(qreal w);

    QColor color() const;
    void setColor(const QColor &c);

    bool aligned() const;
    void setAligned(bool aligned);

    bool isValid() const;

Q_SIGNALS:
    void penChanged();

private:
    qreal m_width;
    QColor m_color;
    bool m_aligned : 1;
    bool m_valid : 1;
};

class Q_AUTOTEST_EXPORT QQuickGradientStop : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal position READ position WRITE setPosition)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    QQuickGradientStop(QObject *parent=0);

    qreal position() const;
    void setPosition(qreal position);

    QColor color() const;
    void setColor(const QColor &color);

private:
    void updateGradient();

private:
    qreal m_position;
    QColor m_color;
};

class Q_AUTOTEST_EXPORT QQuickGradient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDeclarativeListProperty<QQuickGradientStop> stops READ stops)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    QQuickGradient(QObject *parent=0);
    ~QQuickGradient();

    QDeclarativeListProperty<QQuickGradientStop> stops();

    const QGradient *gradient() const;

Q_SIGNALS:
    void updated();

private:
    void doUpdate();

private:
    QList<QQuickGradientStop *> m_stops;
    mutable QGradient *m_gradient;
    friend class QQuickRectangle;
    friend class QQuickGradientStop;
};

class QQuickRectanglePrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickRectangle : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QQuickGradient *gradient READ gradient WRITE setGradient)
    Q_PROPERTY(QQuickPen * border READ border CONSTANT)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
public:
    QQuickRectangle(QQuickItem *parent=0);

    QColor color() const;
    void setColor(const QColor &);

    QQuickPen *border();

    QQuickGradient *gradient() const;
    void setGradient(QQuickGradient *gradient);

    qreal radius() const;
    void setRadius(qreal radius);

    virtual QRectF boundingRect() const;

Q_SIGNALS:
    void colorChanged();
    void radiusChanged();

protected:
    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

private Q_SLOTS:
    void doUpdate();

private:
    Q_DISABLE_COPY(QQuickRectangle)
    Q_DECLARE_PRIVATE(QQuickRectangle)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPen)
QML_DECLARE_TYPE(QQuickGradientStop)
QML_DECLARE_TYPE(QQuickGradient)
QML_DECLARE_TYPE(QQuickRectangle)

QT_END_HEADER

#endif // QQUICKRECTANGLE_P_H
