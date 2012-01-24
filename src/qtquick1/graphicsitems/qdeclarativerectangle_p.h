/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVERECT_H
#define QDECLARATIVERECT_H

#include "qdeclarativeitem.h"

#include <QtGui/qbrush.h>

#include <QtDeclarative/private/qdeclarativeglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class Q_QTQUICK1_EXPORT QDeclarative1Pen : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY penChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY penChanged)
public:
    QDeclarative1Pen(QObject *parent=0)
        : QObject(parent), _width(1), _color("#000000"), _valid(false)
    {}

    int width() const { return _width; }
    void setWidth(int w);

    QColor color() const { return _color; }
    void setColor(const QColor &c);

    bool isValid() { return _valid; }

Q_SIGNALS:
    void penChanged();

private:
    int _width;
    QColor _color;
    bool _valid;
};

class Q_AUTOTEST_EXPORT QDeclarative1GradientStop : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal position READ position WRITE setPosition)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    QDeclarative1GradientStop(QObject *parent=0) : QObject(parent) {}

    qreal position() const { return m_position; }
    void setPosition(qreal position) { m_position = position; updateGradient(); }

    QColor color() const { return m_color; }
    void setColor(const QColor &color) { m_color = color; updateGradient(); }

private:
    void updateGradient();

private:
    qreal m_position;
    QColor m_color;
};

class Q_AUTOTEST_EXPORT QDeclarative1Gradient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDeclarativeListProperty<QDeclarative1GradientStop> stops READ stops)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    QDeclarative1Gradient(QObject *parent=0) : QObject(parent), m_gradient(0) {}
    ~QDeclarative1Gradient() { delete m_gradient; }

    QDeclarativeListProperty<QDeclarative1GradientStop> stops() { return QDeclarativeListProperty<QDeclarative1GradientStop>(this, m_stops); }

    const QGradient *gradient() const;

Q_SIGNALS:
    void updated();

private:
    void doUpdate();

private:
    QList<QDeclarative1GradientStop *> m_stops;
    mutable QGradient *m_gradient;
    friend class QDeclarative1GradientStop;
};

class QDeclarative1RectanglePrivate;
class Q_QTQUICK1_EXPORT QDeclarative1Rectangle : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QDeclarative1Gradient *gradient READ gradient WRITE setGradient)
    Q_PROPERTY(QDeclarative1Pen * border READ border CONSTANT)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
public:
    QDeclarative1Rectangle(QDeclarativeItem *parent=0);

    QColor color() const;
    void setColor(const QColor &);

    QDeclarative1Pen *border();

    QDeclarative1Gradient *gradient() const;
    void setGradient(QDeclarative1Gradient *gradient);

    qreal radius() const;
    void setRadius(qreal radius);

    QRectF boundingRect() const;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

Q_SIGNALS:
    void colorChanged();
    void radiusChanged();

private Q_SLOTS:
    void doUpdate();

private:
    void generateRoundedRect();
    void generateBorderedRect();
    void drawRect(QPainter &painter);

private:
    Q_DISABLE_COPY(QDeclarative1Rectangle)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1Rectangle)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1Pen)
QML_DECLARE_TYPE(QDeclarative1GradientStop)
QML_DECLARE_TYPE(QDeclarative1Gradient)
QML_DECLARE_TYPE(QDeclarative1Rectangle)

QT_END_HEADER

#endif // QDECLARATIVERECT_H
