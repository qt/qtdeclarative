// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qquickscalegrid_p_p.h"

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QQuickScaleGrid
    \brief The QQuickScaleGrid class allows you to specify a 3x3 grid to use in scaling an image.
*/

QQuickScaleGrid::QQuickScaleGrid(QObject *parent) : QObject(parent), _left(0), _top(0), _right(0), _bottom(0)
{
}

bool QQuickScaleGrid::isNull() const
{
    return !_left && !_top && !_right && !_bottom;
}

void QQuickScaleGrid::setLeft(int pos)
{
    if (_left != pos) {
        _left = pos;
        emit leftBorderChanged();
        emit borderChanged();
    }
}

void QQuickScaleGrid::setTop(int pos)
{
    if (_top != pos) {
        _top = pos;
        emit topBorderChanged();
        emit borderChanged();
    }
}

void QQuickScaleGrid::setRight(int pos)
{
    if (_right != pos) {
        _right = pos;
        emit rightBorderChanged();
        emit borderChanged();
    }
}

void QQuickScaleGrid::setBottom(int pos)
{
    if (_bottom != pos) {
        _bottom = pos;
        emit bottomBorderChanged();
        emit borderChanged();
    }
}

QQuickGridScaledImage::QQuickGridScaledImage()
: _l(-1), _r(-1), _t(-1), _b(-1),
  _h(QQuickBorderImage::Stretch), _v(QQuickBorderImage::Stretch)
{
}

QQuickGridScaledImage::QQuickGridScaledImage(const QQuickGridScaledImage &o)
: _l(o._l), _r(o._r), _t(o._t), _b(o._b), _h(o._h), _v(o._v), _pix(o._pix)
{
}

QQuickGridScaledImage &QQuickGridScaledImage::operator=(const QQuickGridScaledImage &o)
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

QQuickGridScaledImage::QQuickGridScaledImage(QIODevice *data)
: _l(-1), _r(-1), _t(-1), _b(-1), _h(QQuickBorderImage::Stretch), _v(QQuickBorderImage::Stretch)
{
    int l = -1;
    int r = -1;
    int t = -1;
    int b = -1;
    QString imgFile;

    QByteArray raw;
    while (raw = data->readLine(), !raw.isEmpty()) {
        QString line = QString::fromUtf8(raw.trimmed());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        int colonId = line.indexOf(QLatin1Char(':'));
        if (colonId <= 0)
            return;

        const QStringView property = QStringView{line}.left(colonId).trimmed();
        QStringView value = QStringView{line}.mid(colonId + 1).trimmed();

        if (property == QLatin1String("border.left")) {
            l = value.toInt();
        } else if (property == QLatin1String("border.right")) {
            r = value.toInt();
        } else if (property == QLatin1String("border.top")) {
            t = value.toInt();
        } else if (property == QLatin1String("border.bottom")) {
            b = value.toInt();
        } else if (property == QLatin1String("source")) {
            if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
                value = value.mid(1, value.size() - 2); // remove leading/trailing quotes.
            imgFile = value.toString();
        } else if (property == QLatin1String("horizontalTileRule") || property == QLatin1String("horizontalTileMode")) {
            _h = stringToRule(value);
        } else if (property == QLatin1String("verticalTileRule") || property == QLatin1String("verticalTileMode")) {
            _v = stringToRule(value);
        }
    }

    if (l < 0 || r < 0 || t < 0 || b < 0 || imgFile.isEmpty())
        return;

    _l = l; _r = r; _t = t; _b = b;
    _pix = imgFile;
}

QQuickBorderImage::TileMode QQuickGridScaledImage::stringToRule(QStringView s)
{
    QStringView string = s;
    if (string.startsWith(QLatin1Char('"')) && string.endsWith(QLatin1Char('"')))
        string = string.mid(1, string.size() - 2); // remove leading/trailing quotes.

    if (string == QLatin1String("Stretch") || string == QLatin1String("BorderImage.Stretch"))
        return QQuickBorderImage::Stretch;
    if (string == QLatin1String("Repeat") || string == QLatin1String("BorderImage.Repeat"))
        return QQuickBorderImage::Repeat;
    if (string == QLatin1String("Round") || string == QLatin1String("BorderImage.Round"))
        return QQuickBorderImage::Round;

    qWarning("QQuickGridScaledImage: Invalid tile rule specified. Using Stretch.");
    return QQuickBorderImage::Stretch;
}

bool QQuickGridScaledImage::isValid() const
{
    return _l >= 0;
}

int QQuickGridScaledImage::gridLeft() const
{
    return _l;
}

int QQuickGridScaledImage::gridRight() const
{
    return _r;
}

int QQuickGridScaledImage::gridTop() const
{
    return _t;
}

int QQuickGridScaledImage::gridBottom() const
{
    return _b;
}

QString QQuickGridScaledImage::pixmapUrl() const
{
    return _pix;
}

QT_END_NAMESPACE

#include "moc_qquickscalegrid_p_p.cpp"
