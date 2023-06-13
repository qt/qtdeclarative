// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickscrollview_p.h"
#include "qquickpane_p_p.h"
#include "qquickscrollbar_p_p.h"

#include <QtQuick/private/qquickflickable_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ScrollView
    \inherits Pane
//!     \instantiates QQuickScrollView
    \inqmlmodule QtQuick.Controls
    \since 5.9
    \ingroup qtquickcontrols-containers
    \ingroup qtquickcontrols-focusscopes
    \brief Scrollable view.

    ScrollView provides scrolling for user-defined content. It can be used to
    either replace a \l Flickable, or to decorate an existing one.

    \image qtquickcontrols-scrollview.png

    The first example demonstrates the simplest usage of ScrollView.

    \snippet qtquickcontrols-scrollview.qml file

    The second example illustrates using an existing \l Flickable, that is,
    a \l ListView.

    \snippet qtquickcontrols-scrollview-listview.qml file

    \note As of Qt-6.0, ScrollView automatically clips its contents if you
    don't use a Flickable as a child. If this is not wanted, you can
    set your own Flickable as a child, and control the \l {Item::}{clip}
    property on the Flickable explicitly.

    \section2 Sizing

    As with Flickable, there are several things to keep in mind when using
    ScrollView:
    \list
        \li If only a single item is used within a ScrollView, the content size is
            automatically calculated based on the implicit size of its contained item.
            However, if more than one item is used (or an implicit size is not
            provided), the \l {QtQuick.Controls::Pane::}{contentWidth} and
            \l {QtQuick.Controls::Pane::}{contentHeight} properties must
            be set to the combined size of its contained items.
        \li If the content size is less than or equal to the size of the ScrollView,
            it will not be scrollable.
        \li If you want the ScrollView to only scroll vertically, you can bind
            \l {QtQuick.Controls::Pane::}{contentWidth} to
            \l {QtQuick.Controls::Control::}{availableWidth}
            (and vice versa for contentHeight). This will let the contents fill
            out all the available space horizontally inside the ScrollView, taking
            any padding or scroll bars into account.
    \endlist

    \section2 Scroll Bars

    The horizontal and vertical scroll bars can be accessed and customized using
    the \l {ScrollBar::horizontal}{ScrollBar.horizontal} and \l {ScrollBar::vertical}
    {ScrollBar.vertical} attached properties. The following example adjusts the scroll
    bar policies so that the horizontal scroll bar is always off, and the vertical
    scroll bar is always on.

    \snippet qtquickcontrols-scrollview-policy.qml file

    \section2 Touch vs. Mouse Interaction

    On touch, ScrollView enables flicking and makes the scroll bars non-interactive.

    \image qtquickcontrols-scrollindicator.gif

    When interacted with a mouse device, flicking is disabled and the scroll bars
    are interactive.

    \image qtquickcontrols-scrollbar.gif

    Scroll bars can be made interactive on touch, or non-interactive when interacted
    with a mouse device, by setting the \l {ScrollBar::}{interactive} property explicitly
    to \c true or \c false, respectively.

    \snippet qtquickcontrols-scrollview-interactive.qml file

    \sa ScrollBar, ScrollIndicator, {Customizing ScrollView}, {Container Controls},
        {Focus Management in Qt Quick Controls}
*/

class QQuickScrollViewPrivate : public QQuickPanePrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickScrollView)

    QQmlListProperty<QObject> contentData() override;
    QQmlListProperty<QQuickItem> contentChildren() override;
    QList<QQuickItem *> contentChildItems() const override;

    QQuickItem *getContentItem() override;

    enum class ContentItemFlag {
        DoNotSet,
        Set
    };

    QQuickFlickable *ensureFlickable(ContentItemFlag contentItemFlag);
    bool setFlickable(QQuickFlickable *flickable, ContentItemFlag contentItemFlag);

    void flickableContentWidthChanged();
    void flickableContentHeightChanged();

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    QQuickScrollBar *verticalScrollBar() const;
    QQuickScrollBar *horizontalScrollBar() const;

    void setScrollBarsInteractive(bool interactive);

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);
    static qsizetype contentData_count(QQmlListProperty<QObject> *prop);
    static QObject *contentData_at(QQmlListProperty<QObject> *prop, qsizetype index);
    static void contentData_clear(QQmlListProperty<QObject> *prop);

    static void contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *obj);
    static qsizetype contentChildren_count(QQmlListProperty<QQuickItem> *prop);
    static QQuickItem *contentChildren_at(QQmlListProperty<QQuickItem> *prop, qsizetype index);
    static void contentChildren_clear(QQmlListProperty<QQuickItem> *prop);

    void itemImplicitWidthChanged(QQuickItem *item) override;

    void updateScrollBarWidth();
    void updateScrollBarHeight();

    void disconnectScrollBarSignals(QQuickScrollBarAttachedPrivate *scrollBar);
    bool wasTouched = false;
    QQuickFlickable *flickable = nullptr;
    bool flickableHasExplicitContentWidth = true;
    bool flickableHasExplicitContentHeight = true;
    bool isUpdatingScrollBar = false;
    qreal effectiveScrollBarWidth = 0;
    qreal effectiveScrollBarHeight = 0;
};

QList<QQuickItem *> QQuickScrollViewPrivate::contentChildItems() const
{
    if (!flickable)
        return QList<QQuickItem *>();

    return flickable->contentItem()->childItems();
}

QQuickItem *QQuickScrollViewPrivate::getContentItem()
{
    if (!contentItem)
        executeContentItem();
    // This function is called by QQuickControl::contentItem() to lazily create
    // a contentItem, so we don't need to try to set it again.
    return ensureFlickable(ContentItemFlag::DoNotSet);
}

QQuickFlickable *QQuickScrollViewPrivate::ensureFlickable(ContentItemFlag contentItemFlag)
{
    Q_Q(QQuickScrollView);
    if (!flickable) {
        flickableHasExplicitContentWidth = false;
        flickableHasExplicitContentHeight = false;
        // Pass ourselves as the Flickable's parent item.
        auto flickable = new QQuickFlickable(q);
        // We almost always want to clip the flickable so that flickable
        // contents doesn't show up outside the scrollview. The only time
        // this is not really needed, is when the scrollview covers the whole
        // window and the scrollbars are transient. But for that corner case, if this
        // optimization is needed, the user can simply create his own flickable
        // child inside the scrollview, and control clipping on it explicit.
        flickable->setClip(true);
        flickable->setPixelAligned(true);
        setFlickable(flickable, contentItemFlag);
    }
    return flickable;
}

void QQuickScrollViewPrivate::updateScrollBarWidth()
{
    Q_Q(QQuickScrollView);
    qreal oldEffectiveScrollBarWidth = effectiveScrollBarWidth;
    if (auto *vBar = verticalScrollBar()) {
        if (vBar->policy() == QQuickScrollBar::AlwaysOff || !vBar->isVisible())
            effectiveScrollBarWidth = 0;
        else
            effectiveScrollBarWidth = vBar->width();
    }
    if (effectiveScrollBarWidth != oldEffectiveScrollBarWidth) {
        if (!isUpdatingScrollBar) {
            QScopedValueRollback<bool> rollback(isUpdatingScrollBar, true);
            emit q->effectiveScrollBarWidthChanged();
        }
    }
}

void QQuickScrollViewPrivate::updateScrollBarHeight()
{
    Q_Q(QQuickScrollView);
    qreal oldEffectiveScrollBarHeight = effectiveScrollBarHeight;
    if (auto *hBar = horizontalScrollBar()) {
        if (hBar->policy() == QQuickScrollBar::AlwaysOff || !hBar->isVisible())
            effectiveScrollBarHeight = 0;
        else
            effectiveScrollBarHeight = hBar->height();
    }
    if (effectiveScrollBarHeight != oldEffectiveScrollBarHeight) {
        if (!isUpdatingScrollBar) {
            QScopedValueRollback<bool> rollback(isUpdatingScrollBar, true);
            emit q->effectiveScrollBarHeightChanged();
        }

    }
}

void QQuickScrollViewPrivate::disconnectScrollBarSignals(QQuickScrollBarAttachedPrivate *scrollBar)
{
    if (!scrollBar)
        return;

    if (scrollBar->vertical) {
        QObjectPrivate::disconnect(scrollBar->vertical, &QQuickScrollBar::policyChanged, this, &QQuickScrollViewPrivate::updateScrollBarWidth);
        QObjectPrivate::disconnect(scrollBar->vertical, &QQuickScrollBar::visibleChanged, this, &QQuickScrollViewPrivate::updateScrollBarWidth);
    }
    if (scrollBar->horizontal) {
        QObjectPrivate::disconnect(scrollBar->horizontal, &QQuickScrollBar::policyChanged, this, &QQuickScrollViewPrivate::updateScrollBarHeight);
        QObjectPrivate::disconnect(scrollBar->horizontal, &QQuickScrollBar::visibleChanged, this, &QQuickScrollViewPrivate::updateScrollBarHeight);
    }
}

bool QQuickScrollViewPrivate::setFlickable(QQuickFlickable *item, ContentItemFlag contentItemFlag)
{
    Q_Q(QQuickScrollView);
    if (item == flickable)
        return false;

    QQuickScrollBarAttached *attached = qobject_cast<QQuickScrollBarAttached *>(qmlAttachedPropertiesObject<QQuickScrollBar>(q, false));

    if (flickable) {
        flickable->removeEventFilter(q);

        if (attached) {
            auto *scrollBar = QQuickScrollBarAttachedPrivate::get(attached);
            scrollBar->setFlickable(nullptr);
            disconnectScrollBarSignals(scrollBar);
        }

        QObjectPrivate::disconnect(flickable->contentItem(), &QQuickItem::childrenChanged, this, &QQuickPanePrivate::contentChildrenChange);
        QObjectPrivate::disconnect(flickable, &QQuickFlickable::contentWidthChanged, this, &QQuickScrollViewPrivate::flickableContentWidthChanged);
        QObjectPrivate::disconnect(flickable, &QQuickFlickable::contentHeightChanged, this, &QQuickScrollViewPrivate::flickableContentHeightChanged);
    }

    flickable = item;
    if (contentItemFlag == ContentItemFlag::Set)
        q->setContentItem(flickable);

    if (flickable) {
        flickable->installEventFilter(q);
        if (hasContentWidth)
            flickable->setContentWidth(contentWidth);
        else
            flickableContentWidthChanged();
        if (hasContentHeight)
            flickable->setContentHeight(contentHeight);
        else
            flickableContentHeightChanged();

        if (attached) {
            auto *scrollBar = QQuickScrollBarAttachedPrivate::get(attached);
            scrollBar->setFlickable(flickable);
            if (scrollBar->vertical) {
                QObjectPrivate::connect(scrollBar->vertical, &QQuickScrollBar::policyChanged, this, &QQuickScrollViewPrivate::updateScrollBarWidth);
                QObjectPrivate::connect(scrollBar->vertical, &QQuickScrollBar::visibleChanged, this, &QQuickScrollViewPrivate::updateScrollBarWidth);
            }
            if (scrollBar->horizontal) {
                QObjectPrivate::connect(scrollBar->horizontal, &QQuickScrollBar::policyChanged, this, &QQuickScrollViewPrivate::updateScrollBarHeight);
                QObjectPrivate::connect(scrollBar->horizontal, &QQuickScrollBar::visibleChanged, this, &QQuickScrollViewPrivate::updateScrollBarHeight);
            }
        }

        QObjectPrivate::connect(flickable->contentItem(), &QQuickItem::childrenChanged, this, &QQuickPanePrivate::contentChildrenChange);
        QObjectPrivate::connect(flickable, &QQuickFlickable::contentWidthChanged, this, &QQuickScrollViewPrivate::flickableContentWidthChanged);
        QObjectPrivate::connect(flickable, &QQuickFlickable::contentHeightChanged, this, &QQuickScrollViewPrivate::flickableContentHeightChanged);
    }

    return true;
}

void QQuickScrollViewPrivate::flickableContentWidthChanged()
{
    Q_Q(QQuickScrollView);
    if (!flickable || !componentComplete)
        return;

    const qreal cw = flickable->contentWidth();
    if (qFuzzyCompare(cw, implicitContentWidth))
        return;

    flickableHasExplicitContentWidth = true;
    implicitContentWidth = cw;
    emit q->implicitContentWidthChanged();
}

void QQuickScrollViewPrivate::flickableContentHeightChanged()
{
    Q_Q(QQuickScrollView);
    if (!flickable || !componentComplete)
        return;

    const qreal ch = flickable->contentHeight();
    if (qFuzzyCompare(ch, implicitContentHeight))
        return;

    flickableHasExplicitContentHeight = true;
    implicitContentHeight = ch;
    emit q->implicitContentHeightChanged();
}

qreal QQuickScrollViewPrivate::getContentWidth() const
{
    if (flickable && flickableHasExplicitContentWidth)
        return flickable->contentWidth();

    // The scrollview wraps a flickable created by us, and nobody searched for it and
    // modified its contentWidth. In that case, since the application does not control
    // this flickable, we fall back to calculate the content width based on the child
    // items inside it.
    return QQuickPanePrivate::getContentWidth();
}

qreal QQuickScrollViewPrivate::getContentHeight() const
{
    if (flickable && flickableHasExplicitContentHeight)
        return flickable->contentHeight();

    // The scrollview wraps a flickable created by us, and nobody searched for it and
    // modified its contentHeight. In that case, since the application does not control
    // this flickable, we fall back to calculate the content height based on the child
    // items inside it.
    return QQuickPanePrivate::getContentHeight();
}

QQuickScrollBar *QQuickScrollViewPrivate::verticalScrollBar() const
{
    Q_Q(const QQuickScrollView);
    QQuickScrollBarAttached *attached = qobject_cast<QQuickScrollBarAttached *>(qmlAttachedPropertiesObject<QQuickScrollBar>(q, false));
    if (!attached)
        return nullptr;
    return attached->vertical();
}

QQuickScrollBar *QQuickScrollViewPrivate::horizontalScrollBar() const
{
    Q_Q(const QQuickScrollView);
    QQuickScrollBarAttached *attached = qobject_cast<QQuickScrollBarAttached *>(qmlAttachedPropertiesObject<QQuickScrollBar>(q, false));
    if (!attached)
        return nullptr;
    return attached->horizontal();
}

void QQuickScrollViewPrivate::setScrollBarsInteractive(bool interactive)
{
    QQuickScrollBar *hbar = horizontalScrollBar();
    if (hbar) {
        QQuickScrollBarPrivate *p = QQuickScrollBarPrivate::get(hbar);
        if (!p->explicitInteractive)
            p->setInteractive(interactive);
    }

    QQuickScrollBar *vbar = verticalScrollBar();
    if (vbar) {
        QQuickScrollBarPrivate *p = QQuickScrollBarPrivate::get(vbar);
        if (!p->explicitInteractive)
            p->setInteractive(interactive);
    }
}

void QQuickScrollViewPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    // If we don't yet have a flickable assigned, and this object is a Flickable,
    // make it our contentItem.
    if (!p->flickable && p->setFlickable(qobject_cast<QQuickFlickable *>(obj), ContentItemFlag::Set))
        return;

    QQuickFlickable *flickable = p->ensureFlickable(ContentItemFlag::Set);
    Q_ASSERT(flickable);
    // Add the object that was declared as a child of us as a child object of the Flickable.
    QQmlListProperty<QObject> data = flickable->flickableData();
    data.append(&data, obj);
}

qsizetype QQuickScrollViewPrivate::contentData_count(QQmlListProperty<QObject> *prop)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        return 0;

    QQmlListProperty<QObject> data = p->flickable->flickableData();
    return data.count(&data);
}

QObject *QQuickScrollViewPrivate::contentData_at(QQmlListProperty<QObject> *prop, qsizetype index)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        return nullptr;

    QQmlListProperty<QObject> data = p->flickable->flickableData();
    return data.at(&data, index);
}

void QQuickScrollViewPrivate::contentData_clear(QQmlListProperty<QObject> *prop)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        return;

    QQmlListProperty<QObject> data = p->flickable->flickableData();
    return data.clear(&data);
}

void QQuickScrollViewPrivate::contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *item)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        p->setFlickable(qobject_cast<QQuickFlickable *>(item), ContentItemFlag::Set);

    QQuickFlickable *flickable = p->ensureFlickable(ContentItemFlag::Set);
    Q_ASSERT(flickable);
    // Add the item that was declared as a child of us as a child item of the Flickable's contentItem.
    QQmlListProperty<QQuickItem> children = flickable->flickableChildren();
    children.append(&children, item);
}

qsizetype QQuickScrollViewPrivate::contentChildren_count(QQmlListProperty<QQuickItem> *prop)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        return 0;

    QQmlListProperty<QQuickItem> children = p->flickable->flickableChildren();
    return children.count(&children);
}

QQuickItem *QQuickScrollViewPrivate::contentChildren_at(QQmlListProperty<QQuickItem> *prop, qsizetype index)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        return nullptr;

    QQmlListProperty<QQuickItem> children = p->flickable->flickableChildren();
    return children.at(&children, index);
}

void QQuickScrollViewPrivate::contentChildren_clear(QQmlListProperty<QQuickItem> *prop)
{
    QQuickScrollViewPrivate *p = static_cast<QQuickScrollViewPrivate *>(prop->data);
    if (!p->flickable)
        return;

    QQmlListProperty<QQuickItem> children = p->flickable->flickableChildren();
    children.clear(&children);
}

void QQuickScrollViewPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    // a special case for width<->height dependent content (wrapping text) in ScrollView
    if (contentWidth < 0 && !componentComplete)
        return;

    QQuickPanePrivate::itemImplicitWidthChanged(item);
}

QQuickScrollView::QQuickScrollView(QQuickItem *parent)
    : QQuickPane(*(new QQuickScrollViewPrivate), parent)
{
    Q_D(QQuickScrollView);
    d->contentWidth = -1;
    d->contentHeight = -1;

    setFiltersChildMouseEvents(true);
    setWheelEnabled(true);
}

QQuickScrollView::~QQuickScrollView()
{
    Q_D(QQuickScrollView);
    QQuickScrollBarAttached *attached = qobject_cast<QQuickScrollBarAttached *>(qmlAttachedPropertiesObject<QQuickScrollBar>(this, false));
    if (attached) {
        auto *scrollBar = QQuickScrollBarAttachedPrivate::get(attached);
        d->disconnectScrollBarSignals(scrollBar);
    }
}

/*!
    \qmlproperty real QtQuick.Controls::ScrollView::effectiveScrollBarWidth
    \since 6.6

    This property holds the effective width of the vertical scrollbar.
    When the scrollbar policy is \c QQuickScrollBar::AlwaysOff or the scrollbar
    is not visible, this property is \c 0.

    \sa {ScrollBar::policy}
*/
qreal QQuickScrollView::effectiveScrollBarWidth()
{
    Q_D(QQuickScrollView);
    return d->effectiveScrollBarWidth;
}

/*!
    \qmlproperty real QtQuick.Controls::ScrollView::effectiveScrollBarHeight
    \since 6.6

    This property holds the effective height of the horizontal scrollbar.
    When the scrollbar policy is \c QQuickScrollBar::AlwaysOff or the scrollbar
    is not visible, this property is \c 0.

    \sa {ScrollBar::policy}
*/
qreal QQuickScrollView::effectiveScrollBarHeight()
{
    Q_D(QQuickScrollView);
    return d->effectiveScrollBarHeight;
}

/*!
    \qmlproperty list<QtObject> QtQuick.Controls::ScrollView::contentData
    \qmldefault

    This property holds the list of content data.

    The list contains all objects that have been declared in QML as children of the view.

    \note Unlike \c contentChildren, \c contentData does include non-visual QML objects.

    \sa Item::data, contentChildren
*/
QQmlListProperty<QObject> QQuickScrollViewPrivate::contentData()
{
    Q_Q(QQuickScrollView);
    return QQmlListProperty<QObject>(q, this,
                                     QQuickScrollViewPrivate::contentData_append,
                                     QQuickScrollViewPrivate::contentData_count,
                                     QQuickScrollViewPrivate::contentData_at,
                                     QQuickScrollViewPrivate::contentData_clear);
}

/*!
    \qmlproperty list<Item> QtQuick.Controls::ScrollView::contentChildren

    This property holds the list of content children.

    The list contains all items that have been declared in QML as children of the view.

    \note Unlike \c contentData, \c contentChildren does not include non-visual QML objects.

    \sa Item::children, contentData
*/
QQmlListProperty<QQuickItem> QQuickScrollViewPrivate::contentChildren()
{
    Q_Q(QQuickScrollView);
    return QQmlListProperty<QQuickItem>(q, this,
                                        QQuickScrollViewPrivate::contentChildren_append,
                                        QQuickScrollViewPrivate::contentChildren_count,
                                        QQuickScrollViewPrivate::contentChildren_at,
                                        QQuickScrollViewPrivate::contentChildren_clear);
}

bool QQuickScrollView::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickScrollView);
    switch (event->type()) {
    case QEvent::TouchBegin:
        d->wasTouched = true;
        d->setScrollBarsInteractive(false);
        return false;

    case QEvent::TouchEnd:
        d->wasTouched = false;
        return false;

    case QEvent::MouseButtonPress:
        // NOTE: Flickable does not handle touch events, only synthesized mouse events
        if (static_cast<QMouseEvent *>(event)->source() == Qt::MouseEventNotSynthesized) {
            d->wasTouched = false;
            d->setScrollBarsInteractive(true);
            return false;
        }
        return !d->wasTouched && item == d->flickable;

    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        if (static_cast<QMouseEvent *>(event)->source() == Qt::MouseEventNotSynthesized)
            return item == d->flickable;
        break;

    case QEvent::HoverEnter:
    case QEvent::HoverMove:
        if (d->wasTouched && (item == d->verticalScrollBar() || item == d->horizontalScrollBar()))
            d->setScrollBarsInteractive(true);
        break;

    default:
        break;
    }

    return false;
}

bool QQuickScrollView::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickScrollView);
    if (event->type() == QEvent::Wheel) {
        d->setScrollBarsInteractive(true);
        if (!d->wheelEnabled) {
            event->ignore();
            return true;
        }
    }
    return QQuickPane::eventFilter(object, event);
}

void QQuickScrollView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickScrollView);
    QQuickPane::keyPressEvent(event);
    switch (event->key()) {
    case Qt::Key_Up:
        if (QQuickScrollBar *vbar = d->verticalScrollBar()) {
            vbar->decrease();
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (QQuickScrollBar *vbar = d->verticalScrollBar()) {
            vbar->increase();
            event->accept();
        }
        break;
    case Qt::Key_Left:
        if (QQuickScrollBar *hbar = d->horizontalScrollBar()) {
            hbar->decrease();
            event->accept();
        }
        break;
    case Qt::Key_Right:
        if (QQuickScrollBar *hbar = d->horizontalScrollBar()) {
            hbar->increase();
            event->accept();
        }
        break;
    default:
        event->ignore();
        break;
    }
}

void QQuickScrollView::componentComplete()
{
    Q_D(QQuickScrollView);
    QQuickPane::componentComplete();
    if (!d->contentItem)
        d->ensureFlickable(QQuickScrollViewPrivate::ContentItemFlag::Set);
}

void QQuickScrollView::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickScrollView);
    if (newItem != d->flickable) {
        // The new flickable was not created by us. In that case, we always
        // assume/require that it has an explicit content size assigned.
        d->flickableHasExplicitContentWidth = true;
        d->flickableHasExplicitContentHeight = true;
        auto newItemAsFlickable = qobject_cast<QQuickFlickable *>(newItem);
        if (newItem && !newItemAsFlickable)
            qmlWarning(this) << "ScrollView only supports Flickable types as its contentItem";
        // This is called by QQuickControlPrivate::setContentItem_helper, so no need to
        // try to set it as the contentItem.
        d->setFlickable(newItemAsFlickable, QQuickScrollViewPrivate::ContentItemFlag::DoNotSet);
        // We do, however, need to set us as its parent item, as setContentItem_helper will only
        // do so if the item doesn't already have a parent. If newItem wasn't declared as our
        // child and was instead imperatively assigned, it may already have a parent item,
        // which we'll need to override.
        if (newItem) {
            newItem->setParentItem(this);

            // Make sure that the scroll bars are stacked in front of the flickable,
            // otherwise events won't get through to them.
            QQuickScrollBar *verticalBar = d->verticalScrollBar();
            if (verticalBar)
                verticalBar->stackAfter(newItem);
            QQuickScrollBar *horizontalBar = d->horizontalScrollBar();
            if (horizontalBar)
                horizontalBar->stackAfter(newItem);
        }
    }
    QQuickPane::contentItemChange(newItem, oldItem);
}

void QQuickScrollView::contentSizeChange(const QSizeF &newSize, const QSizeF &oldSize)
{
    Q_D(QQuickScrollView);
    QQuickPane::contentSizeChange(newSize, oldSize);
    if (d->flickable) {
        // Only set the content size on the flickable if the flickable doesn't
        // have an explicit assignment from before. Otherwise we can end up overwriting
        // assignments done to those properties by the application. The
        // exception is if the application has assigned a content size
        // directly to the scrollview, which will then win even if the
        // application has assigned something else to the flickable.
        if (d->hasContentWidth || !d->flickableHasExplicitContentWidth) {
            d->flickable->setContentWidth(newSize.width());
            d->updateScrollBarWidth();
        }
        if (d->hasContentHeight || !d->flickableHasExplicitContentHeight) {
            d->flickable->setContentHeight(newSize.height());
            d->updateScrollBarHeight();
        }
    }
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickScrollView::accessibleRole() const
{
    return QAccessible::Pane;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickscrollview_p.cpp"
