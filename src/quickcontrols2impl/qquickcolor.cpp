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

#include "qquickcolor_p.h"

QT_BEGIN_NAMESPACE

QQuickColor::QQuickColor(QObject *parent) :
    QObject(parent)
{
}

QColor QQuickColor::transparent(const QColor &color, qreal opacity) const
{
    const auto rgbColor = color.toRgb();
    return QColor(rgbColor.red(), rgbColor.green(), rgbColor.blue(),
                  int(qreal(255) * qBound(qreal(0), opacity, qreal(1))));
}

QColor QQuickColor::blend(const QColor &a, const QColor &b, qreal factor) const
{
    if (factor <= 0.0)
        return a;
    if (factor >= 1.0)
        return b;

    const auto rgbA = a.toRgb();
    const auto rgbB = b.toRgb();
    QColor color;
    color.setRedF(rgbA.redF() * (1.0 - factor) + rgbB.redF() * factor);
    color.setGreenF(rgbA.greenF() * (1.0 - factor) + rgbB.greenF() * factor);
    color.setBlueF(rgbA.blueF() * (1.0 - factor) + rgbB.blueF() * factor);
    return color;
}

QT_END_NAMESPACE

#include "moc_qquickcolor_p.cpp"
