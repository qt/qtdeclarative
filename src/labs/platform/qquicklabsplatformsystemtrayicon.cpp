// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformsystemtrayicon_p.h"
#include "qquicklabsplatformmenu_p.h"
#include "qquicklabsplatformiconloader_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

#include "widgets/qwidgetplatform_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype SystemTrayIcon
    \inherits QtObject
//!     \instantiates QQuickLabsPlatformSystemTrayIcon
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A system tray icon.

    The SystemTrayIcon type provides an icon for an application in the system tray.

    Many desktop platforms provide a special system tray or notification area,
    where applications can display icons and notification messages.

    \image qtlabsplatform-systemtrayicon.png

    The following example shows how to create a system tray icon, and how to make
    use of the \l activated() signal:

    \code
    SystemTrayIcon {
        visible: true
        icon.source: "qrc:/images/tray-icon.png"

        onActivated: {
            window.show()
            window.raise()
            window.requestActivate()
        }
    }
    \endcode

    \section2 Tray menu

    SystemTrayIcon can have a menu that opens when the icon is activated.

    \image qtlabsplatform-systemtrayicon-menu.png

    The following example illustrates how to assign a \l Menu to a system tray icon:

    \code
    SystemTrayIcon {
        visible: true
        icon.source: "qrc:/images/tray-icon.png"

        menu: Menu {
            MenuItem {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }
    }
    \endcode

    \section2 Notification messages

    SystemTrayIcon can display notification messages.

    \image qtlabsplatform-systemtrayicon-message.png

    The following example presents how to show a notification message using
    \l showMessage(), and how to make use of the \l messageClicked() signal:

    \code
    SystemTrayIcon {
        visible: true
        icon.source: "qrc:/images/tray-icon.png"

        onMessageClicked: console.log("Message clicked")
        Component.onCompleted: showMessage("Message title", "Something important came up. Click this to know more.")
    }
    \endcode

    \section2 Availability

    A native system tray icon is currently \l available on the following platforms:

    \list
    \li All window managers and independent tray implementations for X11 that implement the
        \l{http://standards.freedesktop.org/systemtray-spec/systemtray-spec-0.2.html}
        {freedesktop.org XEmbed system tray specification}.
    \li All desktop environments that implement the
        \l{http://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem}
        {freedesktop.org D-Bus StatusNotifierItem specification}, including recent versions of KDE and Unity.
    \li All supported versions of macOS. Note that the Growl notification system must be installed
        for showMessage() to display messages on OS X prior to 10.8 (Mountain Lion).
    \endlist

    \input includes/widgets.qdocinc 1

    \labs

    \sa Menu
*/

/*!
    \qmlsignal Qt.labs.platform::SystemTrayIcon::activated(ActivationReason reason)

    This signal is emitted when the system tray icon is activated by the user. The
    \a reason argument specifies how the system tray icon was activated.

    Available reasons:

    \value SystemTrayIcon.Unknown     Unknown reason
    \value SystemTrayIcon.Context     The context menu for the system tray icon was requested
    \value SystemTrayIcon.DoubleClick The system tray icon was double clicked
    \value SystemTrayIcon.Trigger     The system tray icon was clicked
    \value SystemTrayIcon.MiddleClick The system tray icon was clicked with the middle mouse button
*/

/*!
    \qmlsignal Qt.labs.platform::SystemTrayIcon::messageClicked()

    This signal is emitted when a notification message is clicked by the user.

    \sa showMessage()
*/

Q_LOGGING_CATEGORY(qtLabsPlatformTray, "qt.labs.platform.tray")

QQuickLabsPlatformSystemTrayIcon::QQuickLabsPlatformSystemTrayIcon(QObject *parent)
    : QObject(parent),
      m_complete(false),
      m_visible(false),
      m_menu(nullptr),
      m_iconLoader(nullptr),
      m_handle(nullptr)
{
    m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon();
    if (!m_handle)
        m_handle = QWidgetPlatform::createSystemTrayIcon(this);
    qCDebug(qtLabsPlatformTray) << "SystemTrayIcon ->" << m_handle;

    if (m_handle) {
        connect(m_handle, &QPlatformSystemTrayIcon::activated, this, &QQuickLabsPlatformSystemTrayIcon::activated);
        connect(m_handle, &QPlatformSystemTrayIcon::messageClicked, this, &QQuickLabsPlatformSystemTrayIcon::messageClicked);
    }
}

QQuickLabsPlatformSystemTrayIcon::~QQuickLabsPlatformSystemTrayIcon()
{
    if (m_menu)
        m_menu->setSystemTrayIcon(nullptr);
    cleanup();
    delete m_iconLoader;
    m_iconLoader = nullptr;
    delete m_handle;
    m_handle = nullptr;
}

QPlatformSystemTrayIcon *QQuickLabsPlatformSystemTrayIcon::handle() const
{
    return m_handle;
}

/*!
    \readonly
    \qmlproperty bool Qt.labs.platform::SystemTrayIcon::available

    This property holds whether the system tray is available.
*/
bool QQuickLabsPlatformSystemTrayIcon::isAvailable() const
{
    return m_handle && m_handle->isSystemTrayAvailable();
}

/*!
    \readonly
    \qmlproperty bool Qt.labs.platform::SystemTrayIcon::supportsMessages

    This property holds whether the system tray icon supports notification messages.

    \sa showMessage()
*/
bool QQuickLabsPlatformSystemTrayIcon::supportsMessages() const
{
    return m_handle && m_handle->supportsMessages();
}

/*!
    \qmlproperty bool Qt.labs.platform::SystemTrayIcon::visible

    This property holds whether the system tray icon is visible.

    The default value is \c false.
*/
bool QQuickLabsPlatformSystemTrayIcon::isVisible() const
{
    return m_visible;
}

void QQuickLabsPlatformSystemTrayIcon::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    if (m_handle && m_complete) {
        if (visible)
            init();
        else
            cleanup();
    }

    m_visible = visible;
    emit visibleChanged();
}

/*!
    \qmlproperty string Qt.labs.platform::SystemTrayIcon::tooltip

    This property holds the tooltip of the system tray icon.
*/
QString QQuickLabsPlatformSystemTrayIcon::tooltip() const
{
    return m_tooltip;
}

void QQuickLabsPlatformSystemTrayIcon::setTooltip(const QString &tooltip)
{
    if (m_tooltip == tooltip)
        return;

    if (m_handle && m_complete)
        m_handle->updateToolTip(tooltip);

    m_tooltip = tooltip;
    emit tooltipChanged();
}

/*!
    \qmlproperty Menu Qt.labs.platform::SystemTrayIcon::menu

    This property holds a menu for the system tray icon.
*/
QQuickLabsPlatformMenu *QQuickLabsPlatformSystemTrayIcon::menu() const
{
    return m_menu;
}

void QQuickLabsPlatformSystemTrayIcon::setMenu(QQuickLabsPlatformMenu *menu)
{
    if (m_menu == menu)
        return;

    if (m_menu)
        m_menu->setSystemTrayIcon(nullptr);

    if (menu)
        menu->setSystemTrayIcon(this);

    if (m_handle && m_complete) {
        if (menu && menu->create())
            m_handle->updateMenu(menu->handle());
        else
            m_handle->updateMenu(nullptr);
    }

    m_menu = menu;
    emit menuChanged();
}

/*!
    \since Qt.labs.platform 1.1 (Qt 5.12)
    \qmlproperty rect Qt.labs.platform::SystemTrayIcon::geometry

    This property holds the geometry of the system tray icon.
*/
QRect QQuickLabsPlatformSystemTrayIcon::geometry() const
{
    return m_handle ? m_handle->geometry() : QRect();
}

/*!
    \since Qt.labs.platform 1.1 (Qt 5.12)
    \qmlproperty url Qt.labs.platform::SystemTrayIcon::icon.source
    \qmlproperty string Qt.labs.platform::SystemTrayIcon::icon.name
    \qmlproperty bool Qt.labs.platform::SystemTrayIcon::icon.mask

    This property holds the system tray icon.

    \code
    SystemTrayIcon {
        icon.mask: true
        icon.source: "qrc:/images/tray-icon.png"
    }
    \endcode
*/
QQuickLabsPlatformIcon QQuickLabsPlatformSystemTrayIcon::icon() const
{
    if (!m_iconLoader)
        return QQuickLabsPlatformIcon();

    return m_iconLoader->icon();
}

void QQuickLabsPlatformSystemTrayIcon::setIcon(const QQuickLabsPlatformIcon &icon)
{
    if (iconLoader()->icon() == icon)
        return;

    iconLoader()->setIcon(icon);
    emit iconChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::SystemTrayIcon::show()

    Shows the system tray icon.
*/
void QQuickLabsPlatformSystemTrayIcon::show()
{
    setVisible(true);
}

/*!
    \qmlmethod void Qt.labs.platform::SystemTrayIcon::hide()

    Hides the system tray icon.
*/
void QQuickLabsPlatformSystemTrayIcon::hide()
{
    setVisible(false);
}

/*!
    \qmlmethod void Qt.labs.platform::SystemTrayIcon::showMessage(string title, string message, MessageIcon icon, int msecs)

    Shows a system tray message with the given \a title, \a message and \a icon
    for the time specified in \a msecs.

    \note System tray messages are dependent on the system configuration and user preferences,
    and may not appear at all. Therefore, it should not be relied upon as the sole means for providing
    critical information.

    \sa supportsMessages, messageClicked()
*/
void QQuickLabsPlatformSystemTrayIcon::showMessage(const QString &title, const QString &msg, QPlatformSystemTrayIcon::MessageIcon icon, int msecs)
{
    if (m_handle)
        m_handle->showMessage(title, msg, QIcon(), icon, msecs);
}

void QQuickLabsPlatformSystemTrayIcon::init()
{
    if (!m_handle)
        return;

    m_handle->init();
    if (m_menu && m_menu->create())
        m_handle->updateMenu(m_menu->handle());
    m_handle->updateToolTip(m_tooltip);
    if (m_iconLoader)
        m_iconLoader->setEnabled(true);
}

void QQuickLabsPlatformSystemTrayIcon::cleanup()
{
    if (m_handle)
        m_handle->cleanup();
    if (m_iconLoader)
        m_iconLoader->setEnabled(false);
}

void QQuickLabsPlatformSystemTrayIcon::classBegin()
{
}

void QQuickLabsPlatformSystemTrayIcon::componentComplete()
{
    m_complete = true;
    if (m_visible)
        init();
}

QQuickLabsPlatformIconLoader *QQuickLabsPlatformSystemTrayIcon::iconLoader() const
{
    if (!m_iconLoader) {
        QQuickLabsPlatformSystemTrayIcon *that = const_cast<QQuickLabsPlatformSystemTrayIcon *>(this);
        static int slot = staticMetaObject.indexOfSlot("updateIcon()");
        m_iconLoader = new QQuickLabsPlatformIconLoader(slot, that);
        m_iconLoader->setEnabled(m_complete);
    }
    return m_iconLoader;
}

void QQuickLabsPlatformSystemTrayIcon::updateIcon()
{
    if (!m_handle || !m_iconLoader)
        return;

    const QRect oldGeometry = m_handle->geometry();

    m_handle->updateIcon(m_iconLoader->toQIcon());

    if (oldGeometry != m_handle->geometry())
        emit geometryChanged();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformsystemtrayicon_p.cpp"
