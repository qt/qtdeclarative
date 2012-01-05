/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "QtQuick1/private/qdeclarativescalegrid_p_p.h"

#include <QtDeclarative/qdeclarative.h>

#include <QBuffer>
#include <QDebug>

QT_BEGIN_NAMESPACE


/*!
    \internal
    \class QDeclarative1ScaleGrid
    \brief The QDeclarative1ScaleGrid class allows you to specify a 3x3 grid to use in scaling an image.
*/

QDeclarative1ScaleGrid::QDeclarative1ScaleGrid(QObject *parent) : QObject(parent), _left(0), _top(0), _right(0), _bottom(0)
{
}

QDeclarative1ScaleGrid::~QDeclarative1ScaleGrid()
{
}

bool QDeclarative1ScaleGrid::isNull() const
{
    return !_left && !_top && !_right && !_bottom;
}

void QDeclarative1ScaleGrid::setLeft(int pos)
{
    if (_left != pos) {
        _left = pos;
        emit borderChanged();
    }
}

void QDeclarative1ScaleGrid::setTop(int pos)
{
    if (_top != pos) {
        _top = pos;
        emit borderChanged();
    }
}

void QDeclarative1ScaleGrid::setRight(int pos)
{
    if (_right != pos) {
        _right = pos;
        emit borderChanged();
    }
}

void QDeclarative1ScaleGrid::setBottom(int pos)
{
    if (_bottom != pos) {
        _bottom = pos;
        emit borderChanged();
    }
}

QDeclarative1GridScaledImage::QDeclarative1GridScaledImage()
: _l(-1), _r(-1), _t(-1), _b(-1),
  _h(QDeclarative1BorderImage::Stretch), _v(QDeclarative1BorderImage::Stretch)
{
}

QDeclarative1GridScaledImage::QDeclarative1GridScaledImage(const QDeclarative1GridScaledImage &o)
: _l(o._l), _r(o._r), _t(o._t), _b(o._b), _h(o._h), _v(o._v), _pix(o._pix)
{
}

QDeclarative1GridScaledImage &QDeclarative1GridScaledImage::operator=(const QDeclarative1GridScaledImage &o)
{
    _l = o._l;
    _r = o._r;
    _t = o._t;
    _b = o._b;
    _h = o._h;
    _v = o._v;
    _pix = o._pix;
    return *this;
}

QDeclarative1GridScaledImage::QDeclarative1GridScaledImage(QIODevice *data)
: _l(-1), _r(-1), _t(-1), _b(-1), _h(QDeclarative1BorderImage::Stretch), _v(QDeclarative1BorderImage::Stretch)
{
    int l = -1;
    int r = -1;
    int t = -1;
    int b = -1;
    QString imgFile;

    QByteArray raw;
    while(raw = data->readLine(), !raw.isEmpty()) {
        QString line = QString::fromUtf8(raw.trimmed());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        int colonId = line.indexOf(QLatin1Char(':'));
        if (colonId <= 0)
            return;
        QStringList list;
        list.append(line.left(colonId).trimmed());
        list.append(line.mid(colonId+1).trimmed());

        if (list[0] == QLatin1String("border.left"))
            l = list[1].toInt();
        else if (list[0] == QLatin1String("border.right"))
            r = list[1].toInt();
        else if (list[0] == QLatin1String("border.top"))
            t = list[1].toInt();
        else if (list[0] == QLatin1String("border.bottom"))
            b = list[1].toInt();
        else if (list[0] == QLatin1String("source"))
            imgFile = list[1];
        else if (list[0] == QLatin1String("horizontalTileRule"))
            _h = stringToRule(list[1]);
        else if (list[0] == QLatin1String("verticalTileRule"))
            _v = stringToRule(list[1]);
    }

    if (l < 0 || r < 0 || t < 0 || b < 0 || imgFile.isEmpty())
        return;

    _l = l; _r = r; _t = t; _b = b;

    _pix = imgFile;
    if (_pix.startsWith(QLatin1Char('"')) && _pix.endsWith(QLatin1Char('"')))
        _pix = _pix.mid(1, _pix.size() - 2); // remove leading/trailing quotes.
}

QDeclarative1BorderImage::TileMode QDeclarative1GridScaledImage::stringToRule(const QString &s)
{
    if (s == QLatin1String("Stretch"))
        return QDeclarative1BorderImage::Stretch;
    if (s == QLatin1String("Repeat"))
        return QDeclarative1BorderImage::Repeat;
    if (s == QLatin1String("Round"))
        return QDeclarative1BorderImage::Round;

    qWarning("QDeclarative1GridScaledImage: Invalid tile rule specified. Using Stretch.");
    return QDeclarative1BorderImage::Stretch;
}

bool QDeclarative1GridScaledImage::isValid() const
{
    return _l >= 0;
}

int QDeclarative1GridScaledImage::gridLeft() const
{
    return _l;
}

int QDeclarative1GridScaledImage::gridRight() const
{
    return _r;
}

int QDeclarative1GridScaledImage::gridTop() const
{
    return _t;
}

int QDeclarative1GridScaledImage::gridBottom() const
{
    return _b;
}

QString QDeclarative1GridScaledImage::pixmapUrl() const
{
    return _pix;
}



QT_END_NAMESPACE
