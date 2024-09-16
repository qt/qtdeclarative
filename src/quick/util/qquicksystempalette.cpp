// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicksystempalette_p.h"

#include <QGuiApplication>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickSystemPalettePrivate : public QObjectPrivate
{
public:
    QPalette::ColorGroup group;
};



/*!
    \qmltype SystemPalette
    \nativetype QQuickSystemPalette
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-utility
    \brief Provides access to the Qt palettes.

    The SystemPalette type provides access to the Qt application
    palettes. This provides information about the standard colors used
    for application windows, buttons and other features. These colors
    are grouped into three \e {color groups}: \c active, \c inactive,
    and \c disabled.  See the QPalette documentation for details about
    color groups and the properties provided by SystemPalette.

    This can be used to color items in a way that provides a more
    native look and feel.

    The following example creates a palette from the \c Active color
    group and uses this to color the window and text items
    appropriately:

    \snippet qml/systempalette.qml 0

    \sa QPalette
*/
QQuickSystemPalette::QQuickSystemPalette(QObject *parent)
    : QObject(*(new QQuickSystemPalettePrivate), parent)
{
    Q_D(QQuickSystemPalette);
    d->group = QPalette::Active;
    connect(qApp, SIGNAL(paletteChanged(QPalette)), this, SIGNAL(paletteChanged()));
}

/*!
    \qmlproperty color QtQuick::SystemPalette::window
    The window (general background) color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::window() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Window);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::windowText
    The window text (general foreground) color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::windowText() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::WindowText);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::base
    The base color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::base() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Base);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::text
    The text color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::text() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Text);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::alternateBase
    The alternate base color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::alternateBase() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::AlternateBase);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::button
    The button color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::button() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Button);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::buttonText
    The button text foreground color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::buttonText() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::ButtonText);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::light
    The light color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::light() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Light);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::midlight
    The midlight color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::midlight() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Midlight);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::dark
    The dark color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::dark() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Dark);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::mid
    The mid color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::mid() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Mid);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::shadow
    The shadow color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::shadow() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Shadow);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::highlight
    The highlight color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::highlight() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Highlight);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::highlightedText
    The highlighted text color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::highlightedText() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::HighlightedText);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::placeholderText
    The placeholder text color of the current color group.

    \since 6.2
    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::placeholderText() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::PlaceholderText);
}

/*!
    \qmlproperty color QtQuick::SystemPalette::accent
    The accent color of the current color group.

    \since 6.7
    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::accent() const
{
    Q_D(const QQuickSystemPalette);
    return QGuiApplication::palette().color(d->group, QPalette::Accent);
}

/*!
    \qmlproperty enumeration QtQuick::SystemPalette::colorGroup

    The color group of the palette. This can be one of:

    \value SystemPalette.Active     (default) QPalette::Active
    \value SystemPalette.Inactive   QPalette::Inactive
    \value SystemPalette.Disabled   QPalette::Disabled

    \sa QPalette::ColorGroup
*/
QQuickSystemPalette::ColorGroup QQuickSystemPalette::colorGroup() const
{
    Q_D(const QQuickSystemPalette);
    return (QQuickSystemPalette::ColorGroup)d->group;
}

void QQuickSystemPalette::setColorGroup(QQuickSystemPalette::ColorGroup colorGroup)
{
    Q_D(QQuickSystemPalette);
    d->group = (QPalette::ColorGroup)colorGroup;
    emit paletteChanged();
}

QT_END_NAMESPACE

#include "moc_qquicksystempalette_p.cpp"
