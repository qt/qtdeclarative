/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickicon_p.h"

QT_BEGIN_NAMESPACE

class QQuickIconPrivate : public QSharedData
{
public:
    QQuickIconPrivate()
        : width(0),
          height(0),
          color(Qt::transparent)
    {
    }

    QString name;
    QUrl source;
    int width;
    int height;
    QColor color;
};

QQuickIcon::QQuickIcon()
    : d(new QQuickIconPrivate)
{
}

QQuickIcon::QQuickIcon(const QQuickIcon &other)
    : d(other.d)
{
}

QQuickIcon::~QQuickIcon()
{
}

QQuickIcon &QQuickIcon::operator=(const QQuickIcon &other)
{
    d = other.d;
    return *this;
}

bool QQuickIcon::operator==(const QQuickIcon &other) const
{
    return d == other.d || (d->name == other.d->name
                            && d->source == other.d->source
                            && d->width == other.d->width
                            && d->height == other.d->height
                            && d->color == other.d->color);
}

bool QQuickIcon::operator!=(const QQuickIcon &other) const
{
    return !(*this == other);
}

bool QQuickIcon::isEmpty() const
{
    return d->name.isEmpty() && d->source.isEmpty();
}

QString QQuickIcon::name() const
{
    return d->name;
}

void QQuickIcon::setName(const QString &name)
{
    d->name = name;
}

QUrl QQuickIcon::source() const
{
    return d->source;
}

void QQuickIcon::setSource(const QUrl &source)
{
    d->source = source;
}

int QQuickIcon::width() const
{
    return d->width;
}

void QQuickIcon::setWidth(int width)
{
    d->width = width;
}

int QQuickIcon::height() const
{
    return d->height;
}

void QQuickIcon::setHeight(int height)
{
    d->height = height;
}

QColor QQuickIcon::color() const
{
    return d->color;
}

void QQuickIcon::setColor(const QColor &color)
{
    d->color = color;
}

void QQuickIcon::resetColor()
{
    d->color = Qt::transparent;
}

QT_END_NAMESPACE
