/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        QImage image = d->pix.image();
        if (!image.isNull()) {
            QPainter painter(&image);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(image.rect(), m_color);
            d->pix.setImage(image);
        }
    }
}

QT_END_NAMESPACE

#include "moc_qquickcolorimage_p.cpp"
