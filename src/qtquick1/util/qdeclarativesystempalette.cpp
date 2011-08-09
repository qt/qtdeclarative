/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativesystempalette_p.h"

#include <QApplication>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



class QDeclarative1SystemPalettePrivate : public QObjectPrivate
{
public:
    QPalette palette;
    QPalette::ColorGroup group;
};



/*!
    \qmlclass SystemPalette QDeclarative1SystemPalette
    \inqmlmodule QtQuick 1
    \ingroup qml-utility-elements
    \since QtQuick 1.0
    \brief The SystemPalette element provides access to the Qt palettes.

    The SystemPalette element provides access to the Qt application
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

    \snippet doc/src/snippets/qtquick1/systempalette.qml 0

    \sa QPalette
*/
QDeclarative1SystemPalette::QDeclarative1SystemPalette(QObject *parent)
    : QObject(*(new QDeclarative1SystemPalettePrivate), parent)
{
    Q_D(QDeclarative1SystemPalette);
    d->palette = QApplication::palette();
    d->group = QPalette::Active;
    qApp->installEventFilter(this);
}

QDeclarative1SystemPalette::~QDeclarative1SystemPalette()
{
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::window
    The window (general background) color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::window() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Window);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::windowText
    The window text (general foreground) color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::windowText() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::WindowText);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::base
    The base color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::base() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Base);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::text
    The text color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::text() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Text);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::alternateBase
    The alternate base color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::alternateBase() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::AlternateBase);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::button
    The button color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::button() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Button);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::buttonText
    The button text foreground color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::buttonText() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::ButtonText);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::light
    The light color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::light() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Light);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::midlight
    The midlight color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::midlight() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Midlight);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::dark
    The dark color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::dark() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Dark);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::mid
    The mid color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::mid() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Mid);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::shadow
    The shadow color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::shadow() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Shadow);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::highlight
    The highlight color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::highlight() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::Highlight);
}

/*!
    \qmlproperty color QtQuick1::SystemPalette::highlightedText
    The highlighted text color of the current color group.

    \sa QPalette::ColorRole
*/
QColor QDeclarative1SystemPalette::highlightedText() const
{
    Q_D(const QDeclarative1SystemPalette);
    return d->palette.color(d->group, QPalette::HighlightedText);
}

/*!
    \qmlproperty enumeration QtQuick1::SystemPalette::colorGroup

    The color group of the palette. This can be one of:

    \list
    \o SystemPalette.Active (default)
    \o SystemPalette.Inactive
    \o SystemPalette.Disabled
    \endlist

    \sa QPalette::ColorGroup
*/
QDeclarative1SystemPalette::ColorGroup QDeclarative1SystemPalette::colorGroup() const
{
    Q_D(const QDeclarative1SystemPalette);
    return (QDeclarative1SystemPalette::ColorGroup)d->group;
}

void QDeclarative1SystemPalette::setColorGroup(QDeclarative1SystemPalette::ColorGroup colorGroup)
{
    Q_D(QDeclarative1SystemPalette);
    d->group = (QPalette::ColorGroup)colorGroup;
    emit paletteChanged();
}

bool QDeclarative1SystemPalette::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp) {
        if (event->type() == QEvent::ApplicationPaletteChange) {
            QApplication::postEvent(this, new QEvent(QEvent::ApplicationPaletteChange));
            return false;
        }
    }
    return QObject::eventFilter(watched, event);
}

bool QDeclarative1SystemPalette::event(QEvent *event)
{
    Q_D(QDeclarative1SystemPalette);
    if (event->type() == QEvent::ApplicationPaletteChange) {
        d->palette = QApplication::palette();
        emit paletteChanged();
        return true;
    }
    return QObject::event(event);
}



QT_END_NAMESPACE
