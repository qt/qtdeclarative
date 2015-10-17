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
#include "qquickstyle_p.h"

#include <QtGui/qfont.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

static const QQuickUniversalStyle::Theme DefaultTheme = QQuickUniversalStyle::Light;
static const QQuickUniversalStyle::Accent DefaultAccent = QQuickUniversalStyle::Cobalt;

static QColor qquickuniversal_light_color(QQuickUniversalStyle::SystemColor role)
{
    static const QColor colors[] = {
        "#FFFFFFFF", // SystemAltHighColor
        "#33FFFFFF", // SystemAltLowColor
        "#99FFFFFF", // SystemAltMediumColor
        "#CCFFFFFF", // SystemAltMediumHighColor
        "#66FFFFFF", // SystemAltMediumLowColor
        "#FF000000", // SystemBaseHighColor
        "#33000000", // SystemBaseLowColor
        "#99000000", // SystemBaseMediumColor
        "#CC000000", // SystemBaseMediumHighColor
        "#66000000", // SystemBaseMediumLowColor
        "#FF171717", // SystemChromeAltLowColor
        "#FF000000", // SystemChromeBlackHighColor
        "#33000000", // SystemChromeBlackLowColor
        "#66000000", // SystemChromeBlackMediumLowColor
        "#CC000000", // SystemChromeBlackMediumColor
        "#FFCCCCCC", // SystemChromeDisabledHighColor
        "#FF7A7A7A", // SystemChromeDisabledLowColor
        "#FFCCCCCC", // SystemChromeHighColor
        "#FFF2F2F2", // SystemChromeLowColor
        "#FFE6E6E6", // SystemChromeMediumColor
        "#FFF2F2F2", // SystemChromeMediumLowColor
        "#FFFFFFFF", // SystemChromeWhiteColor
        "#19000000", // SystemListLowColor
        "#33000000"  // SystemListMediumColor
    };
    return colors[role];
}

static QColor qquickuniversal_dark_color(QQuickUniversalStyle::SystemColor role)
{
    static const QColor colors[] = {
        "#FF000000", // SystemAltHighColor
        "#33000000", // SystemAltLowColor
        "#99000000", // SystemAltMediumColor
        "#CC000000", // SystemAltMediumHighColor
        "#66000000", // SystemAltMediumLowColor
        "#FFFFFFFF", // SystemBaseHighColor
        "#33FFFFFF", // SystemBaseLowColor
        "#99FFFFFF", // SystemBaseMediumColor
        "#CCFFFFFF", // SystemBaseMediumHighColor
        "#66FFFFFF", // SystemBaseMediumLowColor
        "#FFF2F2F2", // SystemChromeAltLowColor
        "#FF000000", // SystemChromeBlackHighColor
        "#33000000", // SystemChromeBlackLowColor
        "#66000000", // SystemChromeBlackMediumLowColor
        "#CC000000", // SystemChromeBlackMediumColor
        "#FF333333", // SystemChromeDisabledHighColor
        "#FF858585", // SystemChromeDisabledLowColor
        "#FF767676", // SystemChromeHighColor
        "#FF171717", // SystemChromeLowColor
        "#FF1F1F1F", // SystemChromeMediumColor
        "#FF2B2B2B", // SystemChromeMediumLowColor
        "#FFFFFFFF", // SystemChromeWhiteColor
        "#19FFFFFF", // SystemListLowColor
        "#33FFFFFF"  // SystemListMediumColor
    };
    return colors[role];
}

static QColor qquickuniversal_accent_color(QQuickUniversalStyle::Accent accent)
{
    static const QColor colors[] = {
        "#A4C400", // Lime
        "#60A917", // Green
        "#008A00", // Emerald
        "#00ABA9", // Teal
        "#1BA1E2", // Cyan
        "#3E65FF", // Cobalt
        "#6A00FF", // Indigo
        "#AA00FF", // Violet
        "#F472D0", // Pink
        "#D80073", // Magenta
        "#A20025", // Crimson
        "#E51400", // Red
        "#FA6800", // Orange
        "#F0A30A", // Amber
        "#E3C800", // Yellow
        "#825A2C", // Brown
        "#6D8764", // Olive
        "#647687", // Steel
        "#76608A", // Mauve
        "#87794E"  // Taupe
    };
    return colors[accent];
}

QQuickUniversalStyle::QQuickUniversalStyle(QObject *parent) : QObject(parent),
    m_hasTheme(false), m_hasAccent(false), m_theme(DefaultTheme), m_accent(DefaultAccent)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Parent);
}

QQuickUniversalStyle::~QQuickUniversalStyle()
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Parent);

    reparent(Q_NULLPTR);
}

QQuickUniversalStyle *QQuickUniversalStyle::qmlAttachedProperties(QObject *object)
{
    QQuickUniversalStyle *style = new QQuickUniversalStyle(object);
    QQuickUniversalStyle *parentStyle = QQuickStyle::findParent<QQuickUniversalStyle>(object);
    if (parentStyle)
        style->reparent(parentStyle);

    QList<QQuickUniversalStyle *> children = QQuickStyle::findChildren<QQuickUniversalStyle>(object);
    foreach (QQuickUniversalStyle *child, children)
        child->reparent(style);
    return style;
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
        foreach (QQuickUniversalStyle *child, m_childStyles)
            child->inheritTheme(theme);
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickUniversalStyle::inheritTheme(Theme theme)
{
    if (!m_hasTheme && m_theme != theme) {
        m_theme = theme;
        foreach (QQuickUniversalStyle *child, m_childStyles)
            child->inheritTheme(theme);
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickUniversalStyle::resetTheme()
{
    if (m_hasTheme) {
        m_hasTheme = false;
        QQuickUniversalStyle *parentStyle = QQuickStyle::findParent<QQuickUniversalStyle>(parent());
        inheritTheme(parentStyle ? parentStyle->theme() : DefaultTheme);
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
        foreach (QQuickUniversalStyle *child, m_childStyles)
            child->inheritAccent(accent);
        emit accentChanged();
    }
}

void QQuickUniversalStyle::inheritAccent(Accent accent)
{
    if (!m_hasAccent && m_accent != accent) {
        m_accent = accent;
        foreach (QQuickUniversalStyle *child, m_childStyles)
            child->inheritAccent(accent);
        emit accentChanged();
    }
}

void QQuickUniversalStyle::resetAccent()
{
    if (m_hasAccent) {
        m_hasAccent = false;
        QQuickUniversalStyle *parentStyle = QQuickStyle::findParent<QQuickUniversalStyle>(parent());
        inheritAccent(parentStyle ? parentStyle->accent() : DefaultAccent);
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

int QQuickUniversalStyle::fontSize() const
{
    return 15; // ControlContentThemeFontSize
}

QString QQuickUniversalStyle::fontFamily() const
{
#ifdef Q_OS_WIN
    return QStringLiteral("Segoe UI"); // ContentControlThemeFontFamily
#else
    return QGuiApplication::font().family();
#endif
}

void QQuickUniversalStyle::reparent(QQuickUniversalStyle *style)
{
    if (m_parentStyle != style) {
        if (m_parentStyle)
            m_parentStyle->m_childStyles.remove(this);
        m_parentStyle = style;
        if (style) {
            style->m_childStyles.insert(this);
            inheritTheme(style->theme());
            inheritAccent(style->accent());
        }
    }
}

void QQuickUniversalStyle::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    QQuickUniversalStyle *style = QQuickStyle::instance<QQuickUniversalStyle>(item);
    if (style) {
        QQuickUniversalStyle *parentStyle = QQuickStyle::findParent<QQuickUniversalStyle>(parent);
        if (parentStyle)
            style->reparent(parentStyle);
    }
}

QT_END_NAMESPACE
