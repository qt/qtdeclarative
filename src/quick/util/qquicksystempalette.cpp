/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicksystempalette_p.h"

#include <QGuiApplication>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickSystemPalettePrivate : public QObjectPrivate
{
public:
    QPalette palette;
    QPalette::ColorGroup group;
};



/*!
    \qmltype SystemPalette
    \instantiates QQuickSystemPalette
    \inqmlmodule QtQuick 2
    \ingroup qtquick-visual-utility
    \brief Provides access to the Qt palettes

    The SystemPalette type provides access to the Qt application
    palettes. This provides information about the standard colors used
    for application windows, buttons and other features. These colors
    are grouped into three \e {color groups}: \c Active, \c Inactive,
    and \c Disabled.  See the QPalette documentation for details about
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
    d->palette = QGuiApplication::palette();
    d->group = QPalette::Active;
    qApp->installEventFilter(this);
}

QQuickSystemPalette::~QQuickSystemPalette()
{
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::window
    The window (general background) color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::window() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Window);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::windowText
    The window text (general foreground) color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::windowText() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::WindowText);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::base
    The base color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::base() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Base);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::text
    The text color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::text() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Text);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::alternateBase
    The alternate base color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::alternateBase() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::AlternateBase);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::button
    The button color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::button() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Button);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::buttonText
    The button text foreground color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::buttonText() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::ButtonText);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::light
    The light color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::light() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Light);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::midlight
    The midlight color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::midlight() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Midlight);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::dark
    The dark color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::dark() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Dark);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::mid
    The mid color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::mid() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Mid);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::shadow
    The shadow color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::shadow() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Shadow);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::highlight
    The highlight color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::highlight() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::Highlight);
}

/*!
    \qmlproperty color QtQuick2::SystemPalette::highlightedText
    The highlighted text color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QQuickSystemPalette::highlightedText() const
{
    Q_D(const QQuickSystemPalette);
    return d->palette.color(d->group, QPalette::HighlightedText);
}

/*!
    \qmlproperty enumeration QtQuick2::SystemPalette::colorGroup

    The color group of the palette. This can be one of:

    \list
    \li SystemPalette.Active (default)
    \li SystemPalette.Inactive
    \li SystemPalette.Disabled
    \endlist

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

bool QQuickSystemPalette::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp) {
        if (event->type() == QEvent::ApplicationPaletteChange) {
            QGuiApplication::postEvent(this, new QEvent(QEvent::ApplicationPaletteChange));
            return false;
        }
    }
    return QObject::eventFilter(watched, event);
}

bool QQuickSystemPalette::event(QEvent *event)
{
    Q_D(QQuickSystemPalette);
    if (event->type() == QEvent::ApplicationPaletteChange) {
        d->palette = QGuiApplication::palette();
        emit paletteChanged();
        return true;
    }
    return QObject::event(event);
}

QT_END_NAMESPACE
