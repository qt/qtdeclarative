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

#include <QtCore/private/qobject_p.h>
#include "qquickicon_p.h"

QT_BEGIN_NAMESPACE

class QQuickIconPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickIcon)

public:
    QQuickIconPrivate()
        : width(0),
          height(0),
          color(Qt::transparent)
    {
    }

    QString name;
    QString source;
    int width;
    int height;
    QColor color;
};

QQuickIcon::QQuickIcon(QObject *parent)
    : QObject(*(new QQuickIconPrivate), parent)
{
}

QString QQuickIcon::name() const
{
    Q_D(const QQuickIcon);
    return d->name;
}

void QQuickIcon::setName(const QString &name)
{
    Q_D(QQuickIcon);
    if (name == d->name)
        return;

    d->name = name;
    emit nameChanged();
}

QString QQuickIcon::source() const
{
    Q_D(const QQuickIcon);
    return d->source;
}

void QQuickIcon::setSource(const QString &source)
{
    Q_D(QQuickIcon);
    if (source == d->source)
        return;

    d->source = source;
    emit sourceChanged();
}

int QQuickIcon::width() const
{
    Q_D(const QQuickIcon);
    return d->width;
}

void QQuickIcon::setWidth(int width)
{
    Q_D(QQuickIcon);
    if (width == d->width)
        return;

    d->width = width;
    emit widthChanged();
}

int QQuickIcon::height() const
{
    Q_D(const QQuickIcon);
    return d->height;
}

void QQuickIcon::setHeight(int height)
{
    Q_D(QQuickIcon);
    if (height == d->height)
        return;

    d->height = height;
    emit heightChanged();
}

QColor QQuickIcon::color() const
{
    Q_D(const QQuickIcon);
    return d->color;
}

void QQuickIcon::setColor(const QColor &color)
{
    Q_D(QQuickIcon);
    if (color == d->color)
        return;

    d->color = color;
    emit colorChanged();
}

void QQuickIcon::resetColor()
{
    setColor(Qt::transparent);
}

QT_END_NAMESPACE
