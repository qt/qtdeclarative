// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformmenubar_p.h"
#include "qquicklabsplatformmenu_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuBar
    \inherits QtObject
//!     \instantiates QQuickLabsPlatformMenuBar
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native menubar.

    The MenuBar type provides a QML API for native platform menubars.

    \image qtlabsplatform-menubar.png

    A menubar consists of a list of drop-down menus.

    \code
    MenuBar {
        id: menuBar

        Menu {
            id: fileMenu
            title: qsTr("File")
            // ...
        }

        Menu {
            id: editMenu
            title: qsTr("&Edit")
            // ...
        }

        Menu {
            id: viewMenu
            title: qsTr("&View")
            // ...
        }

        Menu {
            id: helpMenu
            title: qsTr("&Help")
            // ...
        }
    }
    \endcode

    MenuBar is currently available on the following platforms:

    \list
    \li macOS
    \li Android
    \li Linux (only available on desktop environments that provide a global D-Bus menu bar)
    \li Windows
    \endlist

    \labs

    \sa Menu
*/

Q_DECLARE_LOGGING_CATEGORY(qtLabsPlatformMenus)

QQuickLabsPlatformMenuBar::QQuickLabsPlatformMenuBar(QObject *parent)
    : QObject(parent),
      m_complete(false),
      m_window(nullptr),
      m_handle(nullptr)
{
    m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformMenuBar();
    qCDebug(qtLabsPlatformMenus) << "MenuBar ->" << m_handle;
}

QQuickLabsPlatformMenuBar::~QQuickLabsPlatformMenuBar()
{
    for (QQuickLabsPlatformMenu *menu : std::as_const(m_menus))
        menu->setMenuBar(nullptr);
    delete m_handle;
    m_handle = nullptr;
}

QPlatformMenuBar *QQuickLabsPlatformMenuBar::handle() const
{
    return m_handle;
}

/*!
    \qmldefault
    \qmlproperty list<QtObject> Qt.labs.platform::MenuBar::data

    This default property holds the list of all objects declared as children of
    the menubar. The data property includes objects that are not \l Menu instances,
    such as \l Timer and \l QtObject.

    \sa menus
*/
QQmlListProperty<QObject> QQuickLabsPlatformMenuBar::data()
{
    return QQmlListProperty<QObject>(this, nullptr, data_append, data_count, data_at, data_clear);
}

/*!
    \qmlproperty list<Menu> Qt.labs.platform::MenuBar::menus

    This property holds the list of menus in the menubar.
*/
QQmlListProperty<QQuickLabsPlatformMenu> QQuickLabsPlatformMenuBar::menus()
{
    return QQmlListProperty<QQuickLabsPlatformMenu>(this, nullptr, menus_append, menus_count, menus_at, menus_clear);
}

/*!
    \qmlproperty Window Qt.labs.platform::MenuBar::window

    This property holds the menubar's window.

    Unless explicitly set, the window is automatically resolved by iterating
    the QML parent objects until a \l Window or an \l Item that has a window
    is found.
*/
QWindow *QQuickLabsPlatformMenuBar::window() const
{
    return m_window;
}

void QQuickLabsPlatformMenuBar::setWindow(QWindow *window)
{
    if (m_window == window)
        return;

    if (m_handle)
        m_handle->handleReparent(window);

    m_window = window;
    emit windowChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::addMenu(Menu menu)

    Adds a \a menu to end of the menubar.
*/
void QQuickLabsPlatformMenuBar::addMenu(QQuickLabsPlatformMenu *menu)
{
    insertMenu(m_menus.size(), menu);
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::insertMenu(int index, Menu menu)

    Inserts a \a menu at the specified \a index in the menubar.
*/
void QQuickLabsPlatformMenuBar::insertMenu(int index, QQuickLabsPlatformMenu *menu)
{
    if (!menu || m_menus.contains(menu))
        return;

    QQuickLabsPlatformMenu *before = m_menus.value(index);
    m_menus.insert(index, menu);
    m_data.append(menu);
    menu->setMenuBar(this);
    if (m_handle)
        m_handle->insertMenu(menu->create(), before ? before->handle() : nullptr);
    menu->sync();
    emit menusChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::removeMenu(Menu menu)

    Removes a \a menu from the menubar.
*/
void QQuickLabsPlatformMenuBar::removeMenu(QQuickLabsPlatformMenu *menu)
{
    if (!menu || !m_menus.removeOne(menu))
        return;

    m_data.removeOne(menu);
    if (m_handle)
        m_handle->removeMenu(menu->handle());
    menu->setMenuBar(nullptr);
    emit menusChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::clear()

    Removes all menus from the menubar.
*/
void QQuickLabsPlatformMenuBar::clear()
{
    if (m_menus.isEmpty())
        return;

    for (QQuickLabsPlatformMenu *menu : std::as_const(m_menus)) {
        m_data.removeOne(menu);
        if (m_handle)
            m_handle->removeMenu(menu->handle());
        menu->setMenuBar(nullptr);
        delete menu;
    }

    m_menus.clear();
    emit menusChanged();
}

void QQuickLabsPlatformMenuBar::classBegin()
{
}

void QQuickLabsPlatformMenuBar::componentComplete()
{
    m_complete = true;
    for (QQuickLabsPlatformMenu *menu : std::as_const(m_menus))
        menu->sync();
    if (!m_window)
        setWindow(findWindow());
}

QWindow *QQuickLabsPlatformMenuBar::findWindow() const
{
    QObject *obj = parent();
    while (obj) {
        QWindow *window = qobject_cast<QWindow *>(obj);
        if (window)
            return window;
        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window())
            return item->window();
        obj = obj->parent();
    }
    return nullptr;
}

void QQuickLabsPlatformMenuBar::data_append(QQmlListProperty<QObject> *property, QObject *object)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    QQuickLabsPlatformMenu *menu = qobject_cast<QQuickLabsPlatformMenu *>(object);
    if (menu)
        menuBar->addMenu(menu);
    else
        menuBar->m_data.append(object);
}

qsizetype QQuickLabsPlatformMenuBar::data_count(QQmlListProperty<QObject> *property)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    return menuBar->m_data.size();
}

QObject *QQuickLabsPlatformMenuBar::data_at(QQmlListProperty<QObject> *property, qsizetype index)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    return menuBar->m_data.value(index);
}

void QQuickLabsPlatformMenuBar::data_clear(QQmlListProperty<QObject> *property)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    menuBar->m_data.clear();
}

void QQuickLabsPlatformMenuBar::menus_append(QQmlListProperty<QQuickLabsPlatformMenu> *property, QQuickLabsPlatformMenu *menu)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    menuBar->addMenu(menu);
}

qsizetype QQuickLabsPlatformMenuBar::menus_count(QQmlListProperty<QQuickLabsPlatformMenu> *property)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    return menuBar->m_menus.size();
}

QQuickLabsPlatformMenu *QQuickLabsPlatformMenuBar::menus_at(QQmlListProperty<QQuickLabsPlatformMenu> *property, qsizetype index)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    return menuBar->m_menus.value(index);
}

void QQuickLabsPlatformMenuBar::menus_clear(QQmlListProperty<QQuickLabsPlatformMenu> *property)
{
    QQuickLabsPlatformMenuBar *menuBar = static_cast<QQuickLabsPlatformMenuBar *>(property->object);
    menuBar->clear();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformmenubar_p.cpp"
