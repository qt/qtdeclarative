/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include <QtQuick/private/qquickapplication_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qguiapplication.h>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Application
    \instantiates QQuickApplication
    \inqmlmodule QtQuick
    //! once exposed: \inherits CoreApplication?
    //! TODO: \ingroup ?

    \brief Provides access to global application
    state properties shared by many QML components.

    The Application singleton exposes a subset of QApplication's properties to
    QML applications.

    It also provides an aboutToQuit() signal, which is the same as
    QCoreApplication::aboutToQuit().

    \qml
    import QtQuick

    Window {
        id: root
        visible: true
        width: 800
        height: 680

        title: `${Application.name} (${Application.version})`

        Connections {
            target: Application
            function onAboutToQuit() {
                console.log("Bye!")
            }
        }
    }
    \endqml
*/

/*!
    \qmlproperty bool Application::active
    \deprecated [5.2]

    Returns  whether the application is active.
    Use Qt.application.state == Qt.ApplicationActive instead
*/

/*!
    \qmlproperty Qt::ApplicationState Application::state

    This property represents the current state of the application.

    \qml
    Timer {
        interval: 1000; repeat: true
        active: Application.state === Qt.Qt.ApplicationActive
        onTriggered: imageFetcher.fetchLatestImages()
    }
    \endqml
*/

/*!
    \qmlproperty Qt::LayoutDiretcion Application::layoutDirection

    This read-only property can be used to query the default layout
    direction of the application. On system start-up, the default layout
    direction depends on the application's language. The property has a
    value of \c Qt.RightToLeft in locales where text and graphic elements
    are read from right to left, and \c Qt.LeftToRight where the reading
    direction flows from left to right. You can bind to this property to
    customize your application layouts to support both layout directions.

    \qml
    RowLayout {
        layoutDirection: Application.layoutDirection
    }
    \endqml
*/

/*!
    \qmlproperty bool Application::supportsMultipleWindows

    Returns \c true if the platform supports multiple windows. Some embedded
    platforms do not support multiple windows, for example.
 */

/*!
    \qmlproperty QFont Application::font
    Returns the default application font as returned by
    \l QGuiApplication::font().
*/


/*!
    \qmlproperty QString Application::displayName

    This property represents the application display name set on the
    QGuiApplication instance. This property can be written to in order to set
    the application display name.

    \qml
    Binding {
        target: Application
        property: "displayName"
        value: "My Awesome Application"
    }
    \endqml
*/

/*!
    \qmlproperty QQmlListProperty<QQuickScreenInfo> Application::screens

    An array containing the descriptions of all connected screens. The
    elements of the array are objects with the same properties as the
    \l{Screen} attached object. In practice the array corresponds to the screen
    list returned by QGuiApplication::screens(). In addition to examining
    properties like name, width, height, etc., the array elements can also be
    assigned to the screen property of Window items, thus serving as an
    alternative to the C++ side's QWindow::setScreen().

    \sa Screen, Window, {Window::screen}{Window.screen}
*/

/* The following properties are from QQmlApplication.
   ### Document those in QQmlApplication instead once it is exposed
*/

/*!
    \qmlproperty QStringList Application::arguments

    This is a string list of the arguments the executable was invoked with.
 */

/*!
    \qmlproperty QString Application::name

    This is the application name set on the QCoreApplication instance. This
    property can be written to in order to set the application name.
 */

/*!
    \qmlproperty QString Application::version

    This is the application version set on the QCoreApplication instance. This
    property can be written to in order to set the application version.
 */

/*!
    \qmlproperty QString Application::organization

    This is the organization name set on the QCoreApplication instance.
    This property can be written to in order to set the organization name.
 */

/*!
    \qmlproperty QString Application::domain

    This is the organization domain set on the QCoreApplication instance.
    This property can be written to in order to set the organization domain.
 */

/*!
    \qmlsignal Application::aboutToQuit()

    This signal is emitted when the application is about to quit the main
    event loop. The signal is particularly useful if your application has to
    do some last-secondcleanup.
    Note that no user interaction is possible in this state.

    \sa QCoreApplication::aboutToQuit
*/
QQuickApplication::QQuickApplication(QObject *parent)
    : QQmlApplication(parent)
{
    QCoreApplication *app = QCoreApplication::instance();
    if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(app)) {
        connect(guiApp, &QGuiApplication::layoutDirectionChanged,
                this, &QQuickApplication::layoutDirectionChanged);
        connect(guiApp, &QGuiApplication::applicationStateChanged,
                this, &QQuickApplication::stateChanged);
        connect(guiApp, &QGuiApplication::applicationStateChanged,
                this, &QQuickApplication::activeChanged);
        connect(guiApp, &QGuiApplication::applicationDisplayNameChanged,
                this, &QQuickApplication::displayNameChanged);

        connect(guiApp, &QGuiApplication::screenAdded, this, &QQuickApplication::updateScreens);
        connect(guiApp, &QGuiApplication::screenRemoved, this, &QQuickApplication::updateScreens);
        updateScreens();
    }
}

QQuickApplication::~QQuickApplication()
{
}

bool QQuickApplication::active() const
{
    return QGuiApplication::applicationState() == Qt::ApplicationActive;
}

Qt::LayoutDirection QQuickApplication::layoutDirection() const
{
    return QGuiApplication::layoutDirection();
}

bool QQuickApplication::supportsMultipleWindows() const
{
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows);
}

Qt::ApplicationState QQuickApplication::state() const
{
    return QGuiApplication::applicationState();
}

QFont QQuickApplication::font() const
{
    return QGuiApplication::font();
}

QString QQuickApplication::displayName() const
{
    return QGuiApplication::applicationDisplayName();
}

void QQuickApplication::setDisplayName(const QString &displayName)
{
    return QGuiApplication::setApplicationDisplayName(displayName);
}

qsizetype screens_count(QQmlListProperty<QQuickScreenInfo> *prop)
{
    return static_cast<QVector<QQuickScreenInfo *> *>(prop->data)->count();
}

QQuickScreenInfo *screens_at(QQmlListProperty<QQuickScreenInfo> *prop, qsizetype idx)
{
    return static_cast<QVector<QQuickScreenInfo *> *>(prop->data)->at(idx);
}

QQmlListProperty<QQuickScreenInfo> QQuickApplication::screens()
{
    return QQmlListProperty<QQuickScreenInfo>(this,
        const_cast<QVector<QQuickScreenInfo *> *>(&m_screens), &screens_count, &screens_at);
}

void QQuickApplication::updateScreens()
{
    const QList<QScreen *> screenList = QGuiApplication::screens();
    m_screens.resize(screenList.count());
    for (int i = 0; i < screenList.count(); ++i) {
        if (!m_screens[i])
            m_screens[i] = new QQuickScreenInfo(this);
        m_screens[i]->setWrappedScreen(screenList[i]);
    }
    emit screensChanged();
}

QT_END_NAMESPACE

#include "moc_qquickapplication_p.cpp"
