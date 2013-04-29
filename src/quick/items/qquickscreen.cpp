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
    \brief The Screen attached object provides information about the Screen an Item or Window is displayed on.

    The Screen attached object is valid inside Item or Item derived types,
    after component completion. Inside these items it refers to the screen that
    the item is currently being displayed on.

    The attached object is also valid inside Window or Window derived types,
    after component completion. In that case it refers to the screen where the
    Window was created. It is generally better to access the Screen from the
    relevant Item instead, because on a multi-screen desktop computer, the user
    can drag a Window into a position where it spans across multiple screens.
    In that case some Items will be on one screen, and others on a different
    screen.

    To use this type, you will need to import the module with the following line:
    \code
    import QtQuick.Window 2.1
    \endcode
    It is a separate import in order to allow you to have a QML environment
    without access to window system features.

    Note that the Screen type is not valid at Component.onCompleted, because
    the Item or Window has not been displayed on a screen by this time.
*/

/*!
    \qmlattachedproperty String QtQuick.Window2::Screen::name
    \readonly
    \since Qt 5.1

    The name of the screen.
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
    \qmlattachedproperty int QtQuick.Window2::Screen::desktopAvailableWidth
    \readonly
    \since Qt 5.1

    This contains the available width of the collection of screens which make
    up the virtual desktop, in pixels, excluding window manager reserved areas
    such as task bars and system menus. If you want to position a Window at
    the right of the desktop, you can bind to it like this:

    \qml
    x: Screen.desktopAvailableWidth - width
    \endqml
*/
/*!
    \qmlattachedproperty int QtQuick.Window2::Screen::desktopAvailableHeight
    \readonly
    \since Qt 5.1

    This contains the available height of the collection of screens which make
    up the virtual desktop, in pixels, excluding window manager reserved areas
    such as task bars and system menus. If you want to position a Window at
    the bottom of the desktop, you can bind to it like this:

    \qml
    y: Screen.desktopAvailableHeight - height
    \endqml
*/
/*!
    \qmlattachedproperty real QtQuick.Window2::Screen::logicalPixelDensity
    \readonly
    \since Qt 5.1

    The number of logical pixels per millimeter.  Logical pixels are the
    usual units in QML; on some systems they may be different than physical
    pixels.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation QtQuick.Window2::Screen::primaryOrientation
    \readonly

    This contains the primary orientation of the screen.  If the
    screen's height is greater than its width, then the orientation is
    Qt.PortraitOrientation; otherwise it is Qt.LandscapeOrientation.

    If you are designing an application which changes its layout depending on
    device orientation, you probably want to use primaryOrientation to
    determine the layout. That is because on a desktop computer, you can expect
    primaryOrientation to change when the user rotates the screen via the
    operating system's control panel, even if the computer does not contain an
    accelerometer. Likewise on most handheld computers which do have
    accelerometers, the operating system will rotate the whole screen
    automatically, so again you will see the primaryOrientation change.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation QtQuick.Window2::Screen::orientation
    \readonly

    This contains the current orientation of the screen, from the accelerometer
    (if any). On a desktop computer, this value typically does not change.

    If primaryOrientation == orientation, it means that the screen
    automatically rotates all content which is displayed, depending on how you
    hold it. But if orientation changes while primaryOrientation does NOT
    change, then probably you are using a device which does not rotate its own
    display. In that case you may need to use \l Item.rotation or
    \l Item.transform to rotate your content.
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
    } else {
        QQuickWindow *window = qobject_cast<QQuickWindow*>(attachee);
        if (window)
            windowChanged(window);
    }
}

QString QQuickScreenAttached::name() const
{
    if (!m_screen)
        return QString();
    return m_screen->name();
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

int QQuickScreenAttached::desktopAvailableWidth() const
{
    if (!m_screen)
        return 0;
    return m_screen->availableVirtualSize().width();
}

int QQuickScreenAttached::desktopAvailableHeight() const
{
    if (!m_screen)
        return 0;
    return m_screen->availableVirtualSize().height();
}

qreal QQuickScreenAttached::logicalPixelDensity() const
{
    if (!m_screen)
        return 0.0;
    return m_screen->logicalDotsPerInch() / 25.4;
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
        if (!oldScreen || screen->name() != oldScreen->name())
            emit nameChanged();
        if (!oldScreen || screen->orientation() != oldScreen->orientation())
            emit orientationChanged();
        if (!oldScreen || screen->primaryOrientation() != oldScreen->primaryOrientation())
            emit primaryOrientationChanged();
        if (!oldScreen || screen->availableVirtualGeometry() != oldScreen->availableVirtualGeometry())
            emit desktopGeometryChanged();
        if (!oldScreen || screen->logicalDotsPerInch() != oldScreen->logicalDotsPerInch())
            emit logicalPixelDensityChanged();

        connect(screen, SIGNAL(geometryChanged(QRect)),
                this, SIGNAL(widthChanged()));
        connect(screen, SIGNAL(geometryChanged(QRect)),
                this, SIGNAL(heightChanged()));
        connect(screen, SIGNAL(orientationChanged(Qt::ScreenOrientation)),
                this, SIGNAL(orientationChanged()));
        connect(screen, SIGNAL(primaryOrientationChanged(Qt::ScreenOrientation)),
                this, SIGNAL(primaryOrientationChanged()));
        connect(screen, SIGNAL(virtualGeometryChanged(const QRect &)),
                this, SIGNAL(desktopGeometryChanged()));
        connect(screen, SIGNAL(logicalDotsPerInchChanged(qreal)),
                this, SIGNAL(logicalPixelDensityChanged()));
    }
}

QT_END_NAMESPACE
