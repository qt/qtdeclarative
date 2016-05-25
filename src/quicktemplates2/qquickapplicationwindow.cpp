/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#include "qquickapplicationwindow_p.h"
#include "qquickoverlay_p.h"
#include "qquickpopup_p.h"
#include "qquickcontrol_p_p.h"
#include "qquicktextarea_p.h"
#include "qquicktextfield_p.h"
#include "qquicktoolbar_p.h"
#include "qquicktabbar_p.h"

#include <QtCore/private/qobject_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ApplicationWindow
    \inherits Window
    \instantiates QQuickApplicationWindow
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-containers
    \brief Provides a top-level application window.

    ApplicationWindow is a \l Window which makes it convenient to add
    a \l header and \l footer item to the window.

    You can declare ApplicationWindow as the root item of your application,
    and run it by using \l QQmlApplicationEngine.  In this way you can control
    the window's properties, appearance and layout from QML.

    \image qtquickcontrols2-applicationwindow-wireframe.png

    \qml
    import QtQuick.Controls 2.0

    ApplicationWindow {
        visible: true

        header: ToolBar {
            // ...
        }

        footer: TabBar {
            // ...
        }

        StackView {
            anchors.fill: parent
        }
    }
    \endqml

    ApplicationWindow supports popups via its \l overlay property, which
    ensures that popups are displayed above other content and that the
    background is dimmed when a modal popup is visible.

    \note By default, an ApplicationWindow is not visible.

    \sa Page, {Container Controls}
*/

class QQuickApplicationWindowPrivate : public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickApplicationWindow)

public:
    QQuickApplicationWindowPrivate()
        : complete(false)
        , background(nullptr)
        , contentItem(nullptr)
        , header(nullptr)
        , footer(nullptr)
        , overlay(nullptr)
        , activeFocusControl(nullptr)
    { }

    static QQuickApplicationWindowPrivate *get(QQuickApplicationWindow *window)
    {
        return window->d_func();
    }

    void relayout();

    void itemGeometryChanged(QQuickItem *item, const QRectF &newRect, const QRectF &oldRect) override;
    void itemVisibilityChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    void updateFont(const QFont &f);
    inline void setFont_helper(const QFont &f) {
        if (font.resolve() == f.resolve() && font == f)
            return;
        updateFont(f);
    }
    void resolveFont();

    void _q_updateActiveFocus();
    void setActiveFocusControl(QQuickItem *item);

    bool complete;
    QQuickItem *background;
    QQuickItem *contentItem;
    QQuickItem *header;
    QQuickItem *footer;
    QQuickOverlay *overlay;
    QFont font;
    QLocale locale;
    QQuickItem *activeFocusControl;
    QQuickApplicationWindow *q_ptr;
};

void QQuickApplicationWindowPrivate::relayout()
{
    Q_Q(QQuickApplicationWindow);
    QQuickItem *content = q->contentItem();
    qreal hh = header ? header->height() : 0;
    qreal fh = footer ? footer->height() : 0;

    content->setY(hh);
    content->setWidth(q->width());
    content->setHeight(q->height() - hh - fh);

    if (overlay) {
        overlay->setWidth(q->width());
        overlay->setHeight(q->height());
        overlay->stackAfter(content);
    }

    if (header) {
        header->setY(-hh);
        QQuickItemPrivate *p = QQuickItemPrivate::get(header);
        if (!p->widthValid) {
            header->setWidth(q->width());
            p->widthValid = false;
        }
    }

    if (footer) {
        footer->setY(content->height());
        QQuickItemPrivate *p = QQuickItemPrivate::get(footer);
        if (!p->widthValid) {
            footer->setWidth(q->width());
            p->widthValid = false;
        }
    }

    if (background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(background);
        if (!p->widthValid && qFuzzyIsNull(background->x())) {
            background->setWidth(q->width());
            p->widthValid = false;
        }
        if (!p->heightValid && qFuzzyIsNull(background->y())) {
            background->setHeight(q->height());
            p->heightValid = false;
        }
    }
}

void QQuickApplicationWindowPrivate::itemGeometryChanged(QQuickItem *item, const QRectF &newRect, const QRectF &oldRect)
{
    Q_UNUSED(item)
    Q_UNUSED(newRect)
    Q_UNUSED(oldRect)
    relayout();
}

void QQuickApplicationWindowPrivate::itemVisibilityChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickApplicationWindowPrivate::_q_updateActiveFocus()
{
    Q_Q(QQuickApplicationWindow);
    QQuickItem *item = q->activeFocusItem();
    while (item) {
        QQuickControl *control = qobject_cast<QQuickControl *>(item);
        if (control) {
            setActiveFocusControl(control);
            break;
        }
        QQuickTextField *textField = qobject_cast<QQuickTextField *>(item);
        if (textField) {
            setActiveFocusControl(textField);
            break;
        }
        QQuickTextArea *textArea = qobject_cast<QQuickTextArea *>(item);
        if (textArea) {
            setActiveFocusControl(textArea);
            break;
        }
        item = item->parentItem();
    }
}

void QQuickApplicationWindowPrivate::setActiveFocusControl(QQuickItem *control)
{
    Q_Q(QQuickApplicationWindow);
    if (activeFocusControl != control) {
        activeFocusControl = control;
        emit q->activeFocusControlChanged();
    }
}

QQuickApplicationWindow::QQuickApplicationWindow(QWindow *parent) :
    QQuickWindowQmlImpl(parent), d_ptr(new QQuickApplicationWindowPrivate)
{
    d_ptr->q_ptr = this;
    connect(this, SIGNAL(activeFocusItemChanged()), this, SLOT(_q_updateActiveFocus()));
}

QQuickApplicationWindow::~QQuickApplicationWindow()
{
    Q_D(QQuickApplicationWindow);
    if (d->header)
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Visibility |
                                                                    QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
    if (d->footer)
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Visibility |
                                                                    QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
    d_ptr.reset(); // QTBUG-52731
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::background

    This property holds the background item.

    The background item is stacked under the \l {contentItem}{content item},
    but above the \l {Window::color}{background color} of the window.

    \note If the background item has no explicit size specified, it automatically
          follows the control's size. In most cases, there is no need to specify
          width or height for a background item.
*/
QQuickItem *QQuickApplicationWindow::background() const
{
    Q_D(const QQuickApplicationWindow);
    return d->background;
}

void QQuickApplicationWindow::setBackground(QQuickItem *background)
{
    Q_D(QQuickApplicationWindow);
    if (d->background == background)
        return;

    delete d->background;
    d->background = background;
    if (background) {
        background->setParentItem(QQuickWindow::contentItem());
        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);
        if (isComponentComplete())
            d->relayout();
    }
    emit backgroundChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::header

    This property holds the window header item. The header item is positioned to
    the top, and resized to the width of the window. The default value is \c null.

    \note Assigning a ToolBar or TabBar as a window header sets the respective
    \l ToolBar::position or \l TabBar::position property automatically to \c Header.

    \sa footer, Page::header
*/
QQuickItem *QQuickApplicationWindow::header() const
{
    Q_D(const QQuickApplicationWindow);
    return d->header;
}

void QQuickApplicationWindow::setHeader(QQuickItem *header)
{
    Q_D(QQuickApplicationWindow);
    if (d->header == header)
        return;

    if (d->header) {
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Visibility |
                                                                    QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
        d->header->setParentItem(nullptr);
    }
    d->header = header;
    if (header) {
        header->setParentItem(contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(header);
        p->addItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Visibility |
                                 QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
        if (qFuzzyIsNull(header->z()))
            header->setZ(1);
        if (QQuickToolBar *toolBar = qobject_cast<QQuickToolBar *>(header))
            toolBar->setPosition(QQuickToolBar::Header);
        else if (QQuickTabBar *tabBar = qobject_cast<QQuickTabBar *>(header))
            tabBar->setPosition(QQuickTabBar::Header);
    }
    if (isComponentComplete())
        d->relayout();
    emit headerChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::footer

    This property holds the window footer item. The footer item is positioned to
    the bottom, and resized to the width of the window. The default value is \c null.

    \note Assigning a ToolBar or TabBar as a window footer sets the respective
    \l ToolBar::position or \l TabBar::position property automatically to \c Footer.

    \sa header, Page::footer
*/
QQuickItem *QQuickApplicationWindow::footer() const
{
    Q_D(const QQuickApplicationWindow);
    return d->footer;
}

void QQuickApplicationWindow::setFooter(QQuickItem *footer)
{
    Q_D(QQuickApplicationWindow);
    if (d->footer == footer)
        return;

    if (d->footer) {
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Visibility |
                                                                    QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
        d->footer->setParentItem(nullptr);
    }
    d->footer = footer;
    if (footer) {
        footer->setParentItem(contentItem());
        QQuickItemPrivate *p = QQuickItemPrivate::get(footer);
        p->addItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Visibility |
                                 QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
        if (qFuzzyIsNull(footer->z()))
            footer->setZ(1);
        if (QQuickToolBar *toolBar = qobject_cast<QQuickToolBar *>(footer))
            toolBar->setPosition(QQuickToolBar::Footer);
        else if (QQuickTabBar *tabBar = qobject_cast<QQuickTabBar *>(footer))
            tabBar->setPosition(QQuickTabBar::Footer);
    }
    if (isComponentComplete())
        d->relayout();
    emit footerChanged();
}

/*!
    \qmlproperty list<Object> QtQuick.Controls::ApplicationWindow::contentData
    \default

    This default property holds the list of all objects declared as children of
    the window.

    \sa contentItem
*/
QQmlListProperty<QObject> QQuickApplicationWindow::contentData()
{
    return QQuickItemPrivate::get(contentItem())->data();
}

/*!
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::contentItem
    \readonly

    This property holds the window content item.
*/
QQuickItem *QQuickApplicationWindow::contentItem() const
{
    QQuickApplicationWindowPrivate *d = const_cast<QQuickApplicationWindowPrivate *>(d_func());
    if (!d->contentItem) {
        d->contentItem = new QQuickItem(QQuickWindow::contentItem());
        d->contentItem->setFlag(QQuickItem::ItemIsFocusScope);
        d->contentItem->setFocus(true);
        d->relayout();
    }
    return d->contentItem;
}

/*!
    \qmlproperty Control QtQuick.Controls::ApplicationWindow::activeFocusControl
    \readonly

    This property holds the control that currently has active focus, or \c null if there is
    no control with active focus.

    The difference between \l Window::activeFocusItem and ApplicationWindow::activeFocusControl
    is that the former may point to a building block of a control, whereas the latter points
    to the enclosing control. For example, when SpinBox has focus, activeFocusItem points to
    the editor and acticeFocusControl to the SpinBox itself.

    \sa Window::activeFocusItem
*/
QQuickItem *QQuickApplicationWindow::activeFocusControl() const
{
    Q_D(const QQuickApplicationWindow);
    return d->activeFocusControl;
}

/*!
    \qmlpropertygroup QtQuick.Controls::ApplicationWindow::overlay
    \qmlproperty Item QtQuick.Controls::ApplicationWindow::overlay
    \qmlproperty Component QtQuick.Controls::ApplicationWindow::overlay.modal
    \qmlproperty Component QtQuick.Controls::ApplicationWindow::overlay.modeless

    This property holds the window overlay item. Popups are automatically
    reparented to the overlay.

    \table
    \header
        \li Property
        \li Description
    \row
        \li overlay.modal
        \li This property holds a component to use as a visual item that implements
            background dimming for modal popups. It is created for and stacked below
            visible modal popups.
    \row
        \li overlay.modeless
        \li This property holds a component to use as a visual item that implements
            background dimming for modeless popups. It is created for and stacked below
            visible dimming popups.
    \endtable

    \sa Popup::modal, Popup::dim
*/
QQuickOverlay *QQuickApplicationWindow::overlay() const
{
    QQuickApplicationWindowPrivate *d = const_cast<QQuickApplicationWindowPrivate *>(d_func());
    if (!d->overlay) {
        d->overlay = new QQuickOverlay(QQuickWindow::contentItem());
        d->relayout();
    }
    return d->overlay;
}

/*!
    \qmlproperty font QtQuick.Controls::ApplicationWindow::font

    This property holds the font currently set for the window.

    The default font depends on the system environment. QGuiApplication maintains a system/theme
    font which serves as a default for all application windows. You can also set the default font
    for windows by passing a custom font to QGuiApplication::setFont(), before loading any QML.
    Finally, the font is matched against Qt's font database to find the best match.

    ApplicationWindow propagates explicit font properties to child controls. If you change a specific
    property on the window's font, that property propagates to all child controls in the window,
    overriding any system defaults for that property.

    \sa Control::font
*/
QFont QQuickApplicationWindow::font() const
{
    Q_D(const QQuickApplicationWindow);
    return d->font;
}

void QQuickApplicationWindow::setFont(const QFont &font)
{
    Q_D(QQuickApplicationWindow);
    if (d->font.resolve() == font.resolve() && d->font == font)
        return;

    QFont resolvedFont = font.resolve(QQuickControlPrivate::themeFont(QPlatformTheme::SystemFont));
    d->setFont_helper(resolvedFont);
}

void QQuickApplicationWindow::resetFont()
{
    setFont(QFont());
}

void QQuickApplicationWindowPrivate::resolveFont()
{
    QFont resolvedFont = font.resolve(QQuickControlPrivate::themeFont(QPlatformTheme::SystemFont));
    setFont_helper(resolvedFont);
}

void QQuickApplicationWindowPrivate::updateFont(const QFont &f)
{
    Q_Q(QQuickApplicationWindow);
    const bool changed = font != f;
    font = f;

    QQuickControlPrivate::updateFontRecur(q->QQuickWindow::contentItem(), f);

    // TODO: internal QQuickPopupManager that provides reliable access to all QQuickPopup instances
    const QList<QQuickPopup *> popups = q->findChildren<QQuickPopup *>();
    for (QQuickPopup *popup : popups)
        QQuickControlPrivate::get(static_cast<QQuickControl *>(popup->popupItem()))->inheritFont(f);

    if (changed)
        emit q->fontChanged();
}

QLocale QQuickApplicationWindow::locale() const
{
    Q_D(const QQuickApplicationWindow);
    return d->locale;
}

void QQuickApplicationWindow::setLocale(const QLocale &locale)
{
    Q_D(QQuickApplicationWindow);
    if (d->locale == locale)
        return;

    d->locale = locale;
    QQuickControlPrivate::updateLocaleRecur(QQuickWindow::contentItem(), locale);

    // TODO: internal QQuickPopupManager that provides reliable access to all QQuickPopup instances
    const QList<QQuickPopup *> popups = QQuickWindow::contentItem()->findChildren<QQuickPopup *>();
    for (QQuickPopup *popup : popups)
        QQuickControlPrivate::get(static_cast<QQuickControl *>(popup->popupItem()))->updateLocale(locale, false); // explicit=false

    emit localeChanged();
}

void QQuickApplicationWindow::resetLocale()
{
    setLocale(QLocale());
}

QQuickApplicationWindowAttached *QQuickApplicationWindow::qmlAttachedProperties(QObject *object)
{
    return new QQuickApplicationWindowAttached(object);
}

bool QQuickApplicationWindow::isComponentComplete() const
{
    Q_D(const QQuickApplicationWindow);
    return d->complete;
}

void QQuickApplicationWindow::classBegin()
{
    Q_D(QQuickApplicationWindow);
    QQuickWindowQmlImpl::classBegin();
    d->resolveFont();
}

void QQuickApplicationWindow::componentComplete()
{
    Q_D(QQuickApplicationWindow);
    d->complete = true;
    QQuickWindowQmlImpl::componentComplete();
}

void QQuickApplicationWindow::resizeEvent(QResizeEvent *event)
{
    Q_D(QQuickApplicationWindow);
    QQuickWindowQmlImpl::resizeEvent(event);
    d->relayout();
}

class QQuickApplicationWindowAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickApplicationWindowAttached)

public:
    QQuickApplicationWindowAttachedPrivate() : window(nullptr) { }

    void windowChange(QQuickWindow *wnd);

    QQuickApplicationWindow *window;
};

void QQuickApplicationWindowAttachedPrivate::windowChange(QQuickWindow *wnd)
{
    Q_Q(QQuickApplicationWindowAttached);
    if (window && !QQuickApplicationWindowPrivate::get(window))
        window = nullptr; // being deleted (QTBUG-52731)

    QQuickApplicationWindow *newWindow = qobject_cast<QQuickApplicationWindow *>(wnd);
    if (window != newWindow) {
        QQuickApplicationWindow *oldWindow = window;
        if (oldWindow) {
            QObject::disconnect(oldWindow, &QQuickApplicationWindow::activeFocusControlChanged,
                                q, &QQuickApplicationWindowAttached::activeFocusControlChanged);
            QObject::disconnect(oldWindow, &QQuickApplicationWindow::headerChanged,
                                q, &QQuickApplicationWindowAttached::headerChanged);
            QObject::disconnect(oldWindow, &QQuickApplicationWindow::footerChanged,
                                q, &QQuickApplicationWindowAttached::footerChanged);
        }
        if (newWindow) {
            QObject::connect(newWindow, &QQuickApplicationWindow::activeFocusControlChanged,
                             q, &QQuickApplicationWindowAttached::activeFocusControlChanged);
            QObject::connect(newWindow, &QQuickApplicationWindow::headerChanged,
                             q, &QQuickApplicationWindowAttached::headerChanged);
            QObject::connect(newWindow, &QQuickApplicationWindow::footerChanged,
                             q, &QQuickApplicationWindowAttached::footerChanged);
        }

        window = newWindow;
        emit q->windowChanged();
        emit q->contentItemChanged();
        emit q->overlayChanged();

        if ((oldWindow && oldWindow->activeFocusControl()) || (newWindow && newWindow->activeFocusControl()))
            emit q->activeFocusControlChanged();
        if ((oldWindow && oldWindow->header()) || (newWindow && newWindow->header()))
            emit q->headerChanged();
        if ((oldWindow && oldWindow->footer()) || (newWindow && newWindow->footer()))
            emit q->footerChanged();
    }
}

QQuickApplicationWindowAttached::QQuickApplicationWindowAttached(QObject *parent)
    : QObject(*(new QQuickApplicationWindowAttachedPrivate), parent)
{
    Q_D(QQuickApplicationWindowAttached);
    if (QQuickItem *item = qobject_cast<QQuickItem *>(parent)) {
        d->windowChange(item->window());
        QObjectPrivate::connect(item, &QQuickItem::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
        if (!d->window) {
            QQuickItem *p = item;
            while (p) {
                if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(p->parent())) {
                    d->windowChange(popup->window());
                    QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
                }
                p = p->parentItem();
            }
        }
    } else if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(parent)) {
        d->windowChange(popup->window());
        QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickApplicationWindowAttachedPrivate::windowChange);
    }
}

/*!
    \qmlattachedproperty ApplicationWindow QtQuick.Controls::ApplicationWindow::window
    \readonly

    This attached property holds the application window. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.
*/
QQuickApplicationWindow *QQuickApplicationWindowAttached::window() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->window;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::contentItem
    \readonly

    This attached property holds the window content item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.
*/
QQuickItem *QQuickApplicationWindowAttached::contentItem() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->window ? d->window->contentItem() : nullptr;
}

/*!
    \qmlattachedproperty Control QtQuick.Controls::ApplicationWindow::activeFocusControl
    \readonly

    This attached property holds the control that currently has active focus, or \c null
    if there is no control with active focus. The property can be attached to any item.
    The value is \c null if the item is not in an ApplicationWindow, or the window has
    no active focus.

    \sa Window::activeFocusItem
*/
QQuickItem *QQuickApplicationWindowAttached::activeFocusControl() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->window ? d->window->activeFocusControl() : nullptr;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::header
    \readonly

    This attached property holds the window header item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no header item.
*/
QQuickItem *QQuickApplicationWindowAttached::header() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->window ? d->window->header() : nullptr;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::footer
    \readonly

    This attached property holds the window footer item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow, or
    the window has no footer item.
*/
QQuickItem *QQuickApplicationWindowAttached::footer() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->window ? d->window->footer() : nullptr;
}

/*!
    \qmlattachedproperty Item QtQuick.Controls::ApplicationWindow::overlay
    \readonly

    This attached property holds the window overlay item. The property can be attached
    to any item. The value is \c null if the item is not in an ApplicationWindow.
*/
QQuickOverlay *QQuickApplicationWindowAttached::overlay() const
{
    Q_D(const QQuickApplicationWindowAttached);
    return d->window ? d->window->overlay() : nullptr;
}

QT_END_NAMESPACE

#include "moc_qquickapplicationwindow_p.cpp"
