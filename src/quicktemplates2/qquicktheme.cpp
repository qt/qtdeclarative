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

#include "qquicktheme_p.h"
#include "qquicktheme_p_p.h"

#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

QQuickTheme::QQuickTheme()
    : d_ptr(new QQuickThemePrivate)
{
}

QQuickTheme::~QQuickTheme()
{
}

QFont QQuickTheme::themeFont(Font type)
{
    const QFont *font = nullptr;
    if (QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        font = theme->font(type);

    if (font) {
        QFont f = *font;
        if (type == QPlatformTheme::SystemFont)
            f.resolve(0);
        return f;
    }

    return QFont();
}

QPalette QQuickTheme::themePalette(Palette type)
{
    const QPalette *palette = nullptr;
    if (QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        palette = theme->palette(type);

    if (palette) {
        QPalette f = *palette;
        if (type == QPlatformTheme::SystemPalette)
            f.resolve(0);
        return f;
    }

    return QPalette();
}

const QFont *QQuickTheme::font(Font type) const
{
    Q_D(const QQuickTheme);
    Q_UNUSED(type)
    return d->defaultFont.data();
}

const QPalette *QQuickTheme::palette(Palette type) const
{
    Q_D(const QQuickTheme);
    Q_UNUSED(type)
    return d->defaultPalette.data();
}

void QQuickTheme::setDefaultFont(const QFont *defaultFont)
{
    Q_D(QQuickTheme);
    d->defaultFont.reset(defaultFont);
    if (defaultFont)
        resolveFonts(*defaultFont);
}

void QQuickTheme::setDefaultPalette(const QPalette *defaultPalette)
{
    Q_D(QQuickTheme);
    d->defaultPalette.reset(defaultPalette);
    if (defaultPalette)
        resolvePalettes(*defaultPalette);
}

void QQuickTheme::resolveFonts(const QFont &defaultFont)
{
    Q_UNUSED(defaultFont)
}

void QQuickTheme::resolvePalettes(const QPalette &defaultPalette)
{
    Q_UNUSED(defaultPalette)
}

QT_END_NAMESPACE
