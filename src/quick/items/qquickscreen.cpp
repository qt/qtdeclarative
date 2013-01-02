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

#include "qquickscreen_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickwindow.h"

#include <QScreen>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Screen
    \instantiates QQuickScreenAttached
    \inqmlmodule QtQuick.Window 2
    \ingroup qtquick-visual-utility
    \brief The Screen attached object provides information about the Screen an Item is displayed on.

    The Screen attached object is only valid inside Item or Item derived types, after component completion.
    Inside these items it refers to the screen that the item is currently being displayed on.

    To use this type, you will need to import the module with the following line:
    \code
    import QtQuick.Window 2.0
    \endcode

    Note that the Screen type is not valid at Component.onCompleted, because the Item has not been displayed on
    a screen by this time.

    Restricting this import will allow you to have a QML environment without access to window system features.
*/

/*!
    \qmlattachedproperty int QtQuick.Window2::Screen::width
    \readonly

    This contains the width of the screen in pixels.
*/
/*!
    \qmlattachedproperty int QtQuick.Window2::Screen::height
    \readonly

    This contains the height of the screen in pixels.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation QtQuick.Window2::Screen::primaryOrientation
    \readonly

    This contains the primary orientation of the screen.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation QtQuick.Window2::Screen::orientation
    \readonly

    This contains the current orientation of the screen.
*/
/*!
    \qmlattachedmethod int QtQuick.Window2::Screen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)

    Returns the rotation angle, in degrees, between the two specified angles.
*/

QQuickScreenAttached::QQuickScreenAttached(QObject* attachee)
    : QObject(attachee)
    , m_screen(NULL)
    , m_window(NULL)
{
    m_attachee = qobject_cast<QQuickItem*>(attachee);

    if (m_attachee) {
        QQuickItemPrivate::get(m_attachee)->extra.value().screenAttached = this;

        if (m_attachee->window()) //It might not be assigned to a window yet
            windowChanged(m_attachee->window());
    }
}

int QQuickScreenAttached::width() const
{
    if (!m_screen)
        return 0;
    return m_screen->size().width();
}

int QQuickScreenAttached::height() const
{
    if (!m_screen)
        return 0;
    return m_screen->size().height();
}

Qt::ScreenOrientation QQuickScreenAttached::primaryOrientation() const
{
    if (!m_screen)
        return Qt::PrimaryOrientation;
    return m_screen->primaryOrientation();
}

Qt::ScreenOrientation QQuickScreenAttached::orientation() const
{
    if (!m_screen)
        return Qt::PrimaryOrientation;
    return m_screen->orientation();
}

int QQuickScreenAttached::angleBetween(int a, int b)
{
    if (!m_screen)
        return Qt::PrimaryOrientation;
    return m_screen->angleBetween((Qt::ScreenOrientation)a,(Qt::ScreenOrientation)b);
}

void QQuickScreenAttached::windowChanged(QQuickWindow* c)
{
    if (m_window)
        disconnect(m_window, SIGNAL(screenChanged(QScreen*)), this, SLOT(screenChanged(QScreen*)));
    m_window = c;
    screenChanged(c ? c->screen() : NULL);
    if (c)
        connect(c, SIGNAL(screenChanged(QScreen*)), this, SLOT(screenChanged(QScreen*)));
}

void QQuickScreenAttached::screenChanged(QScreen *screen)
{
    //qDebug() << "QQuickScreenAttached::screenChanged" << (screen ? screen->name() : QString::fromLatin1("null"));
    if (screen != m_screen) {
        QScreen* oldScreen = m_screen;
        m_screen = screen;

        if (oldScreen)
            oldScreen->disconnect(this);

        if (!screen)
            return; //Don't bother emitting signals, because the new values are garbage anyways

        if (!oldScreen || screen->size() != oldScreen->size()) {
            emit widthChanged();
            emit heightChanged();
        }

        if (!oldScreen || screen->orientation() != oldScreen->orientation())
            emit orientationChanged();
        if (!oldScreen || screen->primaryOrientation() != oldScreen->primaryOrientation())
            emit primaryOrientationChanged();


        connect(screen, SIGNAL(geometryChanged(QRect)),
                this, SIGNAL(widthChanged()));
        connect(screen, SIGNAL(geometryChanged(QRect)),
                this, SIGNAL(heightChanged()));
        connect(screen, SIGNAL(orientationChanged(Qt::ScreenOrientation)),
                this, SIGNAL(orientationChanged()));
        connect(screen, SIGNAL(primaryOrientationChanged(Qt::ScreenOrientation)),
                this, SIGNAL(primaryOrientationChanged()));
    }
}

QT_END_NAMESPACE
