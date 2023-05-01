// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpageindicator_p.h"
#include "qquickcontrol_p_p.h"

#include <QtCore/qmath.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PageIndicator
    \inherits Control
//!     \instantiates QQuickPageIndicator
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-indicators
    \brief Indicates the currently active page.

    PageIndicator is used to indicate the currently active page
    in a container of multiple pages. PageIndicator consists of
    delegate items that present pages.

    \image qtquickcontrols-pageindicator.png

    \code
    Column {
        StackLayout {
            id: stackLayout

            Page {
                // ...
            }
            Page {
                // ...
            }
            Page {
                // ...
            }
        }

        PageIndicator {
            currentIndex: stackLayout.currentIndex
            count: stackLayout.count
        }
    }
    \endcode

    \sa SwipeView, {Customizing PageIndicator}, {Indicator Controls}
*/

class QQuickPageIndicatorPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickPageIndicator)

public:
    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    QQuickItem *itemAt(const QPointF &pos) const;
    void updatePressed(bool pressed, const QPointF &pos = QPointF());
    void setContextProperty(QQuickItem *item, const QString &name, const QVariant &value);

    void itemChildAdded(QQuickItem *, QQuickItem *child) override;

    int count = 0;
    int currentIndex = 0;
    bool interactive = false;
    QQmlComponent *delegate = nullptr;
    QQuickItem *pressedItem = nullptr;
};

bool QQuickPageIndicatorPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    QQuickControlPrivate::handlePress(point, timestamp);
    if (interactive) {
        updatePressed(true, point);
        return true;
    }
    return false;
}

bool QQuickPageIndicatorPrivate::handleMove(const QPointF &point, ulong timestamp)
{
    QQuickControlPrivate::handleMove(point, timestamp);
    if (interactive) {
        updatePressed(true, point);
        return true;
    }
    return false;
}

bool QQuickPageIndicatorPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickPageIndicator);
    QQuickControlPrivate::handleRelease(point, timestamp);
    if (interactive) {
        if (pressedItem && contentItem)
            q->setCurrentIndex(contentItem->childItems().indexOf(pressedItem));
        updatePressed(false);
        return true;
    }
    return false;
}

void QQuickPageIndicatorPrivate::handleUngrab()
{
    QQuickControlPrivate::handleUngrab();
    if (interactive)
        updatePressed(false);
}

QQuickItem *QQuickPageIndicatorPrivate::itemAt(const QPointF &pos) const
{
    Q_Q(const QQuickPageIndicator);
    if (!contentItem || !q->contains(pos))
        return nullptr;

    QPointF contentPos = q->mapToItem(contentItem, pos);
    QQuickItem *item = contentItem->childAt(contentPos.x(), contentPos.y());
    while (item && item->parentItem() != contentItem)
        item = item->parentItem();
    if (item && !QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return item;

    // find the nearest
    qreal distance = qInf();
    QQuickItem *nearest = nullptr;
    const auto childItems = contentItem->childItems();
    for (QQuickItem *child : childItems) {
        if (QQuickItemPrivate::get(child)->isTransparentForPositioner())
            continue;

        QPointF center = child->boundingRect().center();
        QPointF pt = contentItem->mapToItem(child, contentPos);

        qreal len = QLineF(center, pt).length();
        if (len < distance) {
            distance = len;
            nearest = child;
        }
    }
    return nearest;
}

void QQuickPageIndicatorPrivate::updatePressed(bool pressed, const QPointF &pos)
{
    QQuickItem *prevItem = pressedItem;
    pressedItem = pressed ? itemAt(pos) : nullptr;
    if (prevItem != pressedItem) {
        setContextProperty(prevItem, QStringLiteral("pressed"), false);
        setContextProperty(pressedItem, QStringLiteral("pressed"), pressed);
    }
}

void QQuickPageIndicatorPrivate::setContextProperty(QQuickItem *item, const QString &name, const QVariant &value)
{
    QQmlContext *context = qmlContext(item);
    if (context && context->isValid()) {
        context = context->parentContext();
        if (context && context->isValid())
            context->setContextProperty(name, value);
    }
}

void QQuickPageIndicatorPrivate::itemChildAdded(QQuickItem *, QQuickItem *child)
{
    if (!QQuickItemPrivate::get(child)->isTransparentForPositioner())
        setContextProperty(child, QStringLiteral("pressed"), false);
}

QQuickPageIndicator::QQuickPageIndicator(QQuickItem *parent)
    : QQuickControl(*(new QQuickPageIndicatorPrivate), parent)
{
}

QQuickPageIndicator::~QQuickPageIndicator()
{
    Q_D(QQuickPageIndicator);
    if (d->contentItem)
        QQuickItemPrivate::get(d->contentItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
}

/*!
    \qmlproperty int QtQuick.Controls::PageIndicator::count

    This property holds the number of pages.
*/
int QQuickPageIndicator::count() const
{
    Q_D(const QQuickPageIndicator);
    return d->count;
}

void QQuickPageIndicator::setCount(int count)
{
    Q_D(QQuickPageIndicator);
    if (d->count == count)
        return;

    d->count = count;
    emit countChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::PageIndicator::currentIndex

    This property holds the index of the current page.
*/
int QQuickPageIndicator::currentIndex() const
{
    Q_D(const QQuickPageIndicator);
    return d->currentIndex;
}

void QQuickPageIndicator::setCurrentIndex(int index)
{
    Q_D(QQuickPageIndicator);
    if (d->currentIndex == index)
        return;

    d->currentIndex = index;
    emit currentIndexChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::PageIndicator::interactive

    This property holds whether the control is interactive. An interactive page indicator
    reacts to presses and automatically changes the \l {currentIndex}{current index}
    appropriately.

    \snippet qtquickcontrols-pageindicator-interactive.qml 1

    \note Page indicators are typically quite small (in order to avoid
    distracting the user from the actual content of the user interface). They
    can be hard to click, and might not be easily recognized as interactive by
    the user. For these reasons, they are best used to complement primary
    methods of navigation (such as \l SwipeView), not replace them.

    The default value is \c false.
*/
bool QQuickPageIndicator::isInteractive() const
{
    Q_D(const QQuickPageIndicator);
    return d->interactive;
}

void QQuickPageIndicator::setInteractive(bool interactive)
{
    Q_D(QQuickPageIndicator);
    if (d->interactive == interactive)
        return;

    d->interactive = interactive;
    if (interactive) {
        setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(quicktemplates2_multitouch)
        setAcceptTouchEvents(true);
#endif
#if QT_CONFIG(cursor)
        setCursor(Qt::ArrowCursor);
#endif
    } else {
        setAcceptedMouseButtons(Qt::NoButton);
#if QT_CONFIG(quicktemplates2_multitouch)
        setAcceptTouchEvents(true);
#endif
#if QT_CONFIG(cursor)
        unsetCursor();
#endif
    }
    emit interactiveChanged();
}

/*!
    \qmlproperty Component QtQuick.Controls::PageIndicator::delegate

    This property holds a delegate that presents a page.

    The following properties are available in the context of each delegate:
    \table
        \row \li \b index : int \li The index of the item
        \row \li \b pressed : bool \li Whether the item is pressed
    \endtable
*/
QQmlComponent *QQuickPageIndicator::delegate() const
{
    Q_D(const QQuickPageIndicator);
    return d->delegate;
}

void QQuickPageIndicator::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickPageIndicator);
    if (d->delegate == delegate)
        return;

    d->delegate = delegate;
    emit delegateChanged();
}

void QQuickPageIndicator::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickPageIndicator);
    QQuickControl::contentItemChange(newItem, oldItem);
    if (oldItem)
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
    if (newItem)
        QQuickItemPrivate::get(newItem)->addItemChangeListener(d, QQuickItemPrivate::Children);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickPageIndicator::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickPageIndicator);
    if (d->interactive)
        QQuickControl::touchEvent(event);
    else
        event->ignore(); // QTBUG-61785
}
#endif

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickPageIndicator::accessibleRole() const
{
    return QAccessible::Indicator;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickpageindicator_p.cpp"
