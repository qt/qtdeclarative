/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

#include "qquicksaturationlightnesspicker_p.h"
#include "qquickabstractcolorpicker_p_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickSaturationLightnessPickerPrivate : public QQuickAbstractColorPickerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSaturationLightnessPicker)

public:
    explicit QQuickSaturationLightnessPickerPrivate();
};
QQuickSaturationLightnessPickerPrivate::QQuickSaturationLightnessPickerPrivate()
{
    m_hsl = true;
}

QQuickSaturationLightnessPicker::QQuickSaturationLightnessPicker(QQuickItem *parent)
    : QQuickAbstractColorPicker(*(new QQuickSaturationLightnessPickerPrivate), parent)
{
}

QColor QQuickSaturationLightnessPicker::colorAt(const QPointF &pos)
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0)
        return color();
    const qreal x = qBound(.0, pos.x(), w);
    const qreal y = qBound(.0, pos.y(), h);
    const qreal saturation = 1.0 - (y / h);
    const qreal lightness = x / w;

    return QColor::fromHslF(hue(), saturation, lightness);
}

QT_END_NAMESPACE
