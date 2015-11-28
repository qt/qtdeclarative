/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

#include "qquickuniversalstyle_p.h"

#include <QtCore/qdebug.h>
#include <QtLabsControls/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

static const QQuickUniversalStyle::Theme DefaultTheme = QQuickUniversalStyle::Light;
static const QQuickUniversalStyle::Accent DefaultAccent = QQuickUniversalStyle::Cobalt;

static QColor qquickuniversal_light_color(QQuickUniversalStyle::SystemColor role)
{
    static const QRgb colors[] = {
        0xFFFFFFFF, // SystemAltHighColor
        0x33FFFFFF, // SystemAltLowColor
        0x99FFFFFF, // SystemAltMediumColor
        0xCCFFFFFF, // SystemAltMediumHighColor
        0x66FFFFFF, // SystemAltMediumLowColor
        0xFF000000, // SystemBaseHighColor
        0x33000000, // SystemBaseLowColor
        0x99000000, // SystemBaseMediumColor
        0xCC000000, // SystemBaseMediumHighColor
        0x66000000, // SystemBaseMediumLowColor
        0xFF171717, // SystemChromeAltLowColor
        0xFF000000, // SystemChromeBlackHighColor
        0x33000000, // SystemChromeBlackLowColor
        0x66000000, // SystemChromeBlackMediumLowColor
        0xCC000000, // SystemChromeBlackMediumColor
        0xFFCCCCCC, // SystemChromeDisabledHighColor
        0xFF7A7A7A, // SystemChromeDisabledLowColor
        0xFFCCCCCC, // SystemChromeHighColor
        0xFFF2F2F2, // SystemChromeLowColor
        0xFFE6E6E6, // SystemChromeMediumColor
        0xFFF2F2F2, // SystemChromeMediumLowColor
        0xFFFFFFFF, // SystemChromeWhiteColor
        0x19000000, // SystemListLowColor
        0x33000000  // SystemListMediumColor
    };
    return QColor::fromRgba(colors[role]);
}

static QColor qquickuniversal_dark_color(QQuickUniversalStyle::SystemColor role)
{
    static const QRgb colors[] = {
        0xFF000000, // SystemAltHighColor
        0x33000000, // SystemAltLowColor
        0x99000000, // SystemAltMediumColor
        0xCC000000, // SystemAltMediumHighColor
        0x66000000, // SystemAltMediumLowColor
        0xFFFFFFFF, // SystemBaseHighColor
        0x33FFFFFF, // SystemBaseLowColor
        0x99FFFFFF, // SystemBaseMediumColor
        0xCCFFFFFF, // SystemBaseMediumHighColor
        0x66FFFFFF, // SystemBaseMediumLowColor
        0xFFF2F2F2, // SystemChromeAltLowColor
        0xFF000000, // SystemChromeBlackHighColor
        0x33000000, // SystemChromeBlackLowColor
        0x66000000, // SystemChromeBlackMediumLowColor
        0xCC000000, // SystemChromeBlackMediumColor
        0xFF333333, // SystemChromeDisabledHighColor
        0xFF858585, // SystemChromeDisabledLowColor
        0xFF767676, // SystemChromeHighColor
        0xFF171717, // SystemChromeLowColor
        0xFF1F1F1F, // SystemChromeMediumColor
        0xFF2B2B2B, // SystemChromeMediumLowColor
        0xFFFFFFFF, // SystemChromeWhiteColor
        0x19FFFFFF, // SystemListLowColor
        0x33FFFFFF  // SystemListMediumColor
    };
    return QColor::fromRgba(colors[role]);
}

static QColor qquickuniversal_accent_color(QQuickUniversalStyle::Accent accent)
{
    static const QRgb colors[] = {
        0xA4C400, // Lime
        0x60A917, // Green
        0x008A00, // Emerald
        0x00ABA9, // Teal
        0x1BA1E2, // Cyan
        0x3E65FF, // Cobalt
        0x6A00FF, // Indigo
        0xAA00FF, // Violet
        0xF472D0, // Pink
        0xD80073, // Magenta
        0xA20025, // Crimson
        0xE51400, // Red
        0xFA6800, // Orange
        0xF0A30A, // Amber
        0xE3C800, // Yellow
        0x825A2C, // Brown
        0x6D8764, // Olive
        0x647687, // Steel
        0x76608A, // Mauve
        0x87794E  // Taupe
    };
    return colors[accent];
}

QQuickUniversalStyle::QQuickUniversalStyle(QObject *parent) : QQuickStyle(parent),
    m_hasTheme(false), m_hasAccent(false), m_theme(DefaultTheme), m_accent(DefaultAccent)
{
    init(); // TODO: lazy init?
}

QQuickUniversalStyle *QQuickUniversalStyle::qmlAttachedProperties(QObject *object)
{
    return new QQuickUniversalStyle(object);
}

QQuickUniversalStyle::Theme QQuickUniversalStyle::theme() const
{
    return m_theme;
}

void QQuickUniversalStyle::setTheme(Theme theme)
{
    m_hasTheme = true;
    if (m_theme != theme) {
        m_theme = theme;
        propagateTheme();
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickUniversalStyle::inheritTheme(Theme theme)
{
    if (!m_hasTheme && m_theme != theme) {
        m_theme = theme;
        propagateTheme();
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickUniversalStyle::propagateTheme()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickUniversalStyle *universal = qobject_cast<QQuickUniversalStyle *>(child);
        if (universal)
            universal->inheritTheme(m_theme);
    }
}

void QQuickUniversalStyle::resetTheme()
{
    if (m_hasTheme) {
        m_hasTheme = false;
        QQuickUniversalStyle *universal = qobject_cast<QQuickUniversalStyle *>(parentStyle());
        inheritTheme(universal ? universal->theme() : DefaultTheme);
    }
}

QQuickUniversalStyle::Accent QQuickUniversalStyle::accent() const
{
    return m_accent;
}

void QQuickUniversalStyle::setAccent(Accent accent)
{
    if (accent < Lime || accent > Taupe) {
        qWarning() << "QQuickUniversalStyle: unknown accent" << accent;
        return;
    }
    m_hasAccent = true;
    if (m_accent != accent) {
        m_accent = accent;
        propagateAccent();
        emit accentChanged();
    }
}

void QQuickUniversalStyle::inheritAccent(Accent accent)
{
    if (!m_hasAccent && m_accent != accent) {
        m_accent = accent;
        propagateAccent();
        emit accentChanged();
    }
}

void QQuickUniversalStyle::propagateAccent()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickUniversalStyle *universal = qobject_cast<QQuickUniversalStyle *>(child);
        if (universal)
            universal->inheritAccent(m_accent);
    }
}

void QQuickUniversalStyle::resetAccent()
{
    if (m_hasAccent) {
        m_hasAccent = false;
        QQuickUniversalStyle *universal = qobject_cast<QQuickUniversalStyle *>(parentStyle());
        inheritAccent(universal ? universal->accent() : DefaultAccent);
    }
}

QColor QQuickUniversalStyle::accentColor() const
{
    return qquickuniversal_accent_color(m_accent);
}

QColor QQuickUniversalStyle::altHighColor() const
{
    return getColor(AltHigh);
}

QColor QQuickUniversalStyle::altLowColor() const
{
    return getColor(AltLow);
}

QColor QQuickUniversalStyle::altMediumColor() const
{
    return getColor(AltMedium);
}

QColor QQuickUniversalStyle::altMediumHighColor() const
{
    return getColor(AltMediumHigh);
}

QColor QQuickUniversalStyle::altMediumLowColor() const
{
    return getColor(AltMediumLow);
}

QColor QQuickUniversalStyle::baseHighColor() const
{
    return getColor(BaseHighColor);
}

QColor QQuickUniversalStyle::baseLowColor() const
{
    return getColor(BaseLow);
}

QColor QQuickUniversalStyle::baseMediumColor() const
{
    return getColor(BaseMedium);
}

QColor QQuickUniversalStyle::baseMediumHighColor() const
{
    return getColor(BaseMediumHigh);
}

QColor QQuickUniversalStyle::baseMediumLowColor() const
{
    return getColor(BaseMediumLow);
}

QColor QQuickUniversalStyle::chromeAltLowColor() const
{
    return getColor(ChromeAltLow);
}

QColor QQuickUniversalStyle::chromeBlackHighColor() const
{
    return getColor(ChromeBlackHigh);
}

QColor QQuickUniversalStyle::chromeBlackLowColor() const
{
    return getColor(ChromeBlackLow);
}

QColor QQuickUniversalStyle::chromeBlackMediumLowColor() const
{
    return getColor(ChromeBlackMediumLow);
}

QColor QQuickUniversalStyle::chromeBlackMediumColor() const
{
    return getColor(ChromeBlackMedium);
}

QColor QQuickUniversalStyle::chromeDisabledHighColor() const
{
    return getColor(ChromeDisabledHigh);
}

QColor QQuickUniversalStyle::chromeDisabledLowColor() const
{
    return getColor(ChromeDisabledLow);
}

QColor QQuickUniversalStyle::chromeHighColor() const
{
    return getColor(ChromeHigh);
}

QColor QQuickUniversalStyle::chromeLowColor() const
{
    return getColor(ChromeLow);
}

QColor QQuickUniversalStyle::chromeMediumColor() const
{
    return getColor(ChromeMedium);
}

QColor QQuickUniversalStyle::chromeMediumLowColor() const
{
    return getColor(ChromeMediumLow);
}

QColor QQuickUniversalStyle::chromeWhiteColor() const
{
    return getColor(ChromeWhite);
}

QColor QQuickUniversalStyle::listLowColor() const
{
    return getColor(ListLow);
}

QColor QQuickUniversalStyle::listMediumColor() const
{
    return getColor(ListMedium);
}

QColor QQuickUniversalStyle::getColor(SystemColor role) const
{
    return m_theme == QQuickUniversalStyle::Dark ? qquickuniversal_dark_color(role) : qquickuniversal_light_color(role);
}

void QQuickUniversalStyle::parentStyleChange(QQuickStyle *newParent, QQuickStyle *oldParent)
{
    Q_UNUSED(oldParent);
    QQuickUniversalStyle *universal = qobject_cast<QQuickUniversalStyle *>(newParent);
    if (universal) {
        inheritTheme(universal->theme());
        inheritAccent(universal->accent());
    }
}

QT_END_NAMESPACE
