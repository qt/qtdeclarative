// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    \sa SystemPalette
*/

/*!
    \qmlproperty bool Application::active
    \deprecated [5.2]

    Returns  whether the application is active.
    Use Application.state == Qt.ApplicationActive instead
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
    \qmlproperty Qt::LayoutDirection Application::layoutDirection

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
    \qmlproperty StyleHints Application::styleHints

    The \c styleHints property provides platform-specific style hints and settings.
    See the \l QStyleHints documentation for further details.

    The following example uses \c styleHints to determine whether an
    item should gain focus on mouse press or touch release:
    \code
    import QtQuick

    MouseArea {
        id: button

        onPressed: {
            if (!Application.styleHints.setFocusOnTouchRelease)
                button.forceActiveFocus()
        }
        onReleased: {
            if (Application.styleHints.setFocusOnTouchRelease)
                button.forceActiveFocus()
        }
    }
    \endcode
 */

/*!
    \qmlsignal Application::aboutToQuit()

    This signal is emitted when the application is about to quit the main
    event loop. The signal is particularly useful if your application has to
    do some last-second cleanup. User interaction is not possible in this state.
    For more information, see \l {Window::closing()}{Window.closing}.

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

        connect(guiApp, &QGuiApplication::primaryScreenChanged, this, &QQuickApplication::updateScreens);
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

QStyleHints *QQuickApplication::styleHints()
{
    return QGuiApplication::styleHints();
}

void QQuickApplication::setDisplayName(const QString &displayName)
{
    return QGuiApplication::setApplicationDisplayName(displayName);
}

qsizetype screens_count(QQmlListProperty<QQuickScreenInfo> *prop)
{
    return static_cast<QVector<QQuickScreenInfo *> *>(prop->data)->size();
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
    m_screens.resize(screenList.size());
    for (int i = 0; i < screenList.size(); ++i) {
        if (!m_screens[i])
            m_screens[i] = new QQuickScreenInfo(this);
        m_screens[i]->setWrappedScreen(screenList[i]);
    }
    emit screensChanged();
}

QT_END_NAMESPACE

#include "moc_qquickapplication_p.cpp"
