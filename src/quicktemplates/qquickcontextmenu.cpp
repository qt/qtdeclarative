// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcontextmenu_p.h"

#include <QtCore/qpointer.h>
#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcContextMenu, "qt.quick.controls.contextmenu")

/*!
    \qmltype ContextMenu
    \brief The ContextMenu attached type provides a way to open a context menu
        in a platform-appropriate manner.
    \inqmlmodule QtQuick.Controls
    \ingroup qtquickcontrols-menus
    \since 6.9

    ContextMenu can be attached to any \l {QQuickItem}{item} in order to show a context menu
    upon a platform-specific event, such as a right click or the context menu
    key.

    \snippet qtquickcontrols-contextmenu.qml root

    \section1 Sharing context menus

    It's possible to share a \l Menu amongst several attached context menu
    objects. This allows reusing a single Menu when the items that need
    context menus have data in common. For example:

    \snippet qtquickcontrols-contextmenu-shared.qml file

    \section1 Performance

    ContextMenu lazily creates its \c Menu only when it's requested. If it
    wasn't for this optimization, the \c Menu would be created when the
    containing component is being loaded, which is typically at application
    startup.

    It is recommended not to give the \c Menu assigned to ContextMenu's \l menu
    property an id when it's defined where it's assigned. Doing so prevents
    this optimization. For example:

    \snippet qtquickcontrols-contextmenu-id.qml root

    The example in the \l {Sharing context menus} section works because the
    \c Menu is defined separately from its assignment.

    \section1 Interaction with other menus

    If a \c Menu is opened via e.g. a \l TapHandler or other means, ContextMenu
    will not open at the same time. This allows legacy applications that were
    written before ContextMenu was introduced to continue working as expected.
*/

/*!
    \qmlsignal QtQuick.Controls::ContextMenu::requested(point position)

    This signal is emitted when a context menu is requested.

    If it was requested by a right mouse button click, \a position gives the
    position of the click relative to the parent.

    The example below shows how to programmatically open a context menu:

    \snippet qtquickcontrols-contextmenu-onrequested.qml buttonAndMenu

    If no menu is set, but this signal is connected, the context menu event
    will be accepted and will not propagate.

    \sa QContextMenuEvent::pos()
*/

class QQuickContextMenuPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickContextMenu)

    static QQuickContextMenuPrivate *get(QQuickContextMenu *attachedObject)
    {
        return attachedObject->d_func();
    }

    void cancelMenu();
    void executeMenu(bool complete = false);

    bool isRequestedSignalConnected();

    QQuickDeferredPointer<QQuickMenu> menu;
    bool complete = false;
};

static const QString menuPropertyName = QStringLiteral("menu");

void QQuickContextMenuPrivate::cancelMenu()
{
    Q_Q(QQuickContextMenu);
    quickCancelDeferred(q, menuPropertyName);
}

void QQuickContextMenuPrivate::executeMenu(bool complete)
{
    Q_Q(QQuickContextMenu);
    if (menu.wasExecuted())
        return;

    QQmlEngine *engine = nullptr;
    auto *parentItem = qobject_cast<QQuickItem *>(q->parent());
    if (parentItem) {
        engine = qmlEngine(parentItem);
        // In most cases, the above line will work, but if it doesn't,
        // it could be because we're attached to the contentItem of the window.
        // In that case, we'll be created before the contentItem has a context
        // and hence an engine. However, the window will have one, so use that.
        if (!engine)
            engine = qmlEngine(parentItem->window());
    }

    if (!menu || complete)
        quickBeginAttachedDeferred(q, menuPropertyName, menu, engine);
    if (complete)
        quickCompleteAttachedDeferred(q, menuPropertyName, menu, engine);
}

bool QQuickContextMenuPrivate::isRequestedSignalConnected()
{
    Q_Q(QQuickContextMenu);
    IS_SIGNAL_CONNECTED(q, QQuickContextMenu, requested, (QPointF));
}

QQuickContextMenu::QQuickContextMenu(QObject *parent)
    : QObject(*(new QQuickContextMenuPrivate), parent)
{
    Q_ASSERT(parent);
    if (parent->isQuickItemType()) {
        auto *itemPriv = QQuickItemPrivate::get(static_cast<QQuickItem *>(parent));
        Q_ASSERT(itemPriv);
        if (QObject *oldMenu = itemPriv->setContextMenu(this))
            qCWarning(lcContextMenu) << this << "replaced" << oldMenu << "on" << parent;
    } else {
        qmlWarning(parent) << "ContextMenu must be attached to an Item";
    }
}

QQuickContextMenu *QQuickContextMenu::qmlAttachedProperties(QObject *object)
{
    return new QQuickContextMenu(object);
}

/*!
    \qmlproperty Menu QtQuick.Controls::ContextMenu::menu

    This property holds the context menu that will be opened. It can be set to
    any \l Menu object.

    \note The \l Menu assigned to this property cannot be given an id. See
    \l {Sharing context menus} for more information.
*/
QQuickMenu *QQuickContextMenu::menu() const
{
    auto *d = const_cast<QQuickContextMenuPrivate *>(d_func());
    if (!d->menu) {
        qCDebug(lcContextMenu) << "creating menu via deferred execution"
            << "- is component complete:" << d->complete;
        d->executeMenu(d->complete);
    }
    return d->menu;
}

void QQuickContextMenu::setMenu(QQuickMenu *menu)
{
    Q_D(QQuickContextMenu);
    if (!parent()->isQuickItemType())
        return;

    if (menu == d->menu)
        return;

    if (!d->menu.isExecuting())
        d->cancelMenu();

    d->menu = menu;

    if (!d->menu.isExecuting())
        emit menuChanged();
}

void QQuickContextMenu::classBegin()
{
}

void QQuickContextMenu::componentComplete()
{
    Q_D(QQuickContextMenu);
    d->complete = true;
}

bool QQuickContextMenu::event(QEvent *event)
{
    Q_D(QQuickContextMenu);
    switch (event->type()) {
    case QEvent::ContextMenu: {
        qCDebug(lcContextMenu) << this << "handling" << event << "on behalf of" << parent();

        auto *attacheeItem = qobject_cast<QQuickItem *>(parent());
        Q_ASSERT(attacheeItem);
        const auto *contextMenuEvent = static_cast<QContextMenuEvent *>(event);
        const QPoint posRelativeToParent(attacheeItem->mapFromScene(contextMenuEvent->pos()).toPoint());

        const bool isRequestedSignalConnected = d->isRequestedSignalConnected();
        if (isRequestedSignalConnected)
            Q_EMIT requested(posRelativeToParent);

        auto *menu = this->menu();
        if (!menu) {
            if (isRequestedSignalConnected) {
                qCDebug(lcContextMenu) << this << "no menu instance but accepting event anyway"
                    << "since requested signal has connections";
                event->accept();
                return true;
            }

            // No menu set and requested isn't connected; let the event propagate
            // onwards and do nothing.
            return QObject::event(event);
        }

        menu->setParentItem(attacheeItem);

        qCDebug(lcContextMenu) << this << "showing" << menu << "at" << posRelativeToParent;
        menu->popup(posRelativeToParent);
        event->accept();
        return true;
    }
    default:
        break;
    }
    return QObject::event(event);
}

QT_END_NAMESPACE

#include "moc_qquickcontextmenu_p.cpp"
