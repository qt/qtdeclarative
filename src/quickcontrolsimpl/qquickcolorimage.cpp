// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickcolorimage_p.h"

#include <QtQuick/private/qquickimagebase_p_p.h>

QT_BEGIN_NAMESPACE

QQuickColorImage::QQuickColorImage(QQuickItem *parent)
    : QQuickImage(parent)
{
}

QColor QQuickColorImage::color() const
{
    return m_color;
}

void QQuickColorImage::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    if (isComponentComplete())
        load();
    emit colorChanged();
}

void QQuickColorImage::resetColor()
{
    setColor(Qt::transparent);
}

/*!
    \internal

    This property holds the original color of the image.

    This allows avoiding colorization if the \c color set by the style or user
    is the same as the default color.
*/
QColor QQuickColorImage::defaultColor() const
{
    return m_defaultColor;
}

void QQuickColorImage::setDefaultColor(const QColor &color)
{
    if (m_defaultColor == color)
        return;

    m_defaultColor = color;
    emit defaultColorChanged();
}

void QQuickColorImage::resetDefaultColor()
{
    setDefaultColor(Qt::transparent);
}

void QQuickColorImage::pixmapChange()
{
    QQuickImage::pixmapChange();
    if (m_color.alpha() > 0 && m_color != m_defaultColor) {
        QQuickImageBasePrivate *d = static_cast<QQuickImageBasePrivate *>(QQuickItemPrivate::get(this));
        QImage image = d->currentPix->image();
        if (!image.isNull()) {
            QPainter painter(&image);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(image.rect(), m_color);
            d->currentPix->setImage(image);
        }
    }
}

QT_END_NAMESPACE

#include "moc_qquickcolorimage_p.cpp"
