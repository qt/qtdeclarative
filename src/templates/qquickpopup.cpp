/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickpopup_p.h"
#include "qquickpopup_p_p.h"
#include "qquickapplicationwindow_p.h"
#include "qquickoverlay_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Popup
    \inherits QtObject
    \instantiates QQuickPopup
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-popups
    \brief A popup control.

    Popup is the base type of popup-like user interface controls.

    \labs
*/

static const QQuickItemPrivate::ChangeTypes AncestorChangeTypes = QQuickItemPrivate::Geometry
                                                                  | QQuickItemPrivate::Parent
                                                                  | QQuickItemPrivate::Children;

static const QQuickItemPrivate::ChangeTypes ItemChangeTypes = QQuickItemPrivate::Geometry
                                                             | QQuickItemPrivate::Parent
                                                             | QQuickItemPrivate::Destroyed;

QQuickPopupPrivate::QQuickPopupPrivate()
    : QObjectPrivate()
    , focus(false)
    , modal(false)
    , hasTopPadding(false)
    , hasLeftPadding(false)
    , hasRightPadding(false)
    , hasBottomPadding(false)
    , padding(0)
    , topPadding(0)
    , leftPadding(0)
    , rightPadding(0)
    , bottomPadding(0)
    , contentWidth(0)
    , contentHeight(0)
    , parentItem(Q_NULLPTR)
    , background(Q_NULLPTR)
    , contentItem(Q_NULLPTR)
    , overlay(Q_NULLPTR)
    , enter(Q_NULLPTR)
    , exit(Q_NULLPTR)
    , popupItem(Q_NULLPTR)
    , positioner(this)
    , transitionManager(this)
{
}

void QQuickPopupPrivate::init()
{
    Q_Q(QQuickPopup);
    popupItem = new QQuickPopupItem(q);
    popupItem->setParent(q);
    q->setParentItem(qobject_cast<QQuickItem *>(parent));
}

void QQuickPopupPrivate::finalizeEnterTransition()
{
    if (focus)
        popupItem->setFocus(true);
}

void QQuickPopupPrivate::finalizeExitTransition()
{
    Q_Q(QQuickPopup);
    overlay = Q_NULLPTR;
    positioner.setParentItem(Q_NULLPTR);
    popupItem->setParentItem(Q_NULLPTR);
    emit q->visibleChanged();
}

void QQuickPopupPrivate::resizeBackground()
{
    Q_Q(QQuickPopup);
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

void QQuickPopupPrivate::resizeContent()
{
    Q_Q(QQuickPopup);
    if (contentItem) {
        contentItem->setPosition(QPointF(q->leftPadding(), q->topPadding()));
        contentItem->setSize(QSizeF(q->availableWidth(), q->availableHeight()));
    }
}

void QQuickPopupPrivate::setTopPadding(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldPadding = q->topPadding();
    topPadding = value;
    hasTopPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->topPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, oldPadding, rightPadding, bottomPadding));
    }
}

void QQuickPopupPrivate::setLeftPadding(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldPadding = q->leftPadding();
    leftPadding = value;
    hasLeftPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->leftPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(oldPadding, topPadding, rightPadding, bottomPadding));
    }
}

void QQuickPopupPrivate::setRightPadding(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldPadding = q->rightPadding();
    rightPadding = value;
    hasRightPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->rightPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, topPadding, oldPadding, bottomPadding));
    }
}

void QQuickPopupPrivate::setBottomPadding(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldPadding = q->bottomPadding();
    bottomPadding = value;
    hasBottomPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->bottomPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, topPadding, rightPadding, oldPadding));
    }
}

class QQuickPopupItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupItem)

public:
    QQuickPopupItemPrivate(QQuickPopup *popup);

    void implicitWidthChanged() Q_DECL_OVERRIDE;
    void implicitHeightChanged() Q_DECL_OVERRIDE;

    QQuickPopup *popup;
};

QQuickPopupItemPrivate::QQuickPopupItemPrivate(QQuickPopup *popup) : popup(popup)
{
}

void QQuickPopupItemPrivate::implicitWidthChanged()
{
    emit popup->implicitHeightChanged();
}

void QQuickPopupItemPrivate::implicitHeightChanged()
{
    emit popup->implicitHeightChanged();
}

QQuickPopupItem::QQuickPopupItem(QQuickPopup *popup) :
    QQuickItem(*(new QQuickPopupItemPrivate(popup)))
{
    setAcceptedMouseButtons(Qt::AllButtons);
}

void QQuickPopupItem::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->focusInEvent(event);
}

void QQuickPopupItem::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->focusOutEvent(event);
}

void QQuickPopupItem::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->keyPressEvent(event);
}

void QQuickPopupItem::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->keyReleaseEvent(event);
}

void QQuickPopupItem::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mousePressEvent(event);
}

void QQuickPopupItem::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mouseMoveEvent(event);
}

void QQuickPopupItem::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mouseReleaseEvent(event);
}

void QQuickPopupItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mouseDoubleClickEvent(event);
}

void QQuickPopupItem::mouseUngrabEvent()
{
    Q_D(QQuickPopupItem);
    d->popup->mouseUngrabEvent();
}

void QQuickPopupItem::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->wheelEvent(event);
}

void QQuickPopupItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPopupItem);
    d->popup->geometryChanged(newGeometry, oldGeometry);
}

QQuickPopupPositioner::QQuickPopupPositioner(QQuickPopupPrivate *popup) :
    m_x(0),
    m_y(0),
    m_parentItem(Q_NULLPTR),
    m_popup(popup)
{
}

QQuickPopupPositioner::~QQuickPopupPositioner()
{
    if (m_parentItem) {
        QQuickItemPrivate::get(m_parentItem)->removeItemChangeListener(this, ItemChangeTypes);
        removeAncestorListeners(m_parentItem->parentItem());
    }
}

qreal QQuickPopupPositioner::x() const
{
    return m_x;
}

void QQuickPopupPositioner::setX(qreal x)
{
    if (m_x != x) {
        m_x = x;
        repositionPopup();
    }
}

qreal QQuickPopupPositioner::y() const
{
    return m_y;
}

void QQuickPopupPositioner::setY(qreal y)
{
    if (m_y != y) {
        m_y = y;
        repositionPopup();
    }
}

QQuickItem *QQuickPopupPositioner::parentItem() const
{
    return m_parentItem;
}

void QQuickPopupPositioner::setParentItem(QQuickItem *parent)
{
    if (m_parentItem == parent)
        return;

    if (m_parentItem) {
        QQuickItemPrivate::get(m_parentItem)->removeItemChangeListener(this, ItemChangeTypes);
        removeAncestorListeners(m_parentItem->parentItem());
    }

    m_parentItem = parent;

    if (!parent)
        return;

    QQuickItemPrivate::get(parent)->addItemChangeListener(this, ItemChangeTypes);
    addAncestorListeners(parent->parentItem());

    repositionPopup();
}

void QQuickPopupPositioner::itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &)
{
    repositionPopup();
}

void QQuickPopupPositioner::itemParentChanged(QQuickItem *, QQuickItem *parent)
{
    addAncestorListeners(parent);
}

void QQuickPopupPositioner::itemChildRemoved(QQuickItem *, QQuickItem *child)
{
    if (isAncestor(child))
        removeAncestorListeners(child);
}

void QQuickPopupPositioner::itemDestroyed(QQuickItem *item)
{
    Q_ASSERT(m_parentItem == item);

    m_parentItem = Q_NULLPTR;
    QQuickItemPrivate::get(item)->removeItemChangeListener(this, ItemChangeTypes);
    removeAncestorListeners(item->parentItem());
}

void QQuickPopupPositioner::repositionPopup()
{
    QRectF rect(m_x, m_y, m_popup->popupItem->width(), m_popup->popupItem->height());
    if (m_parentItem) {
        rect = m_parentItem->mapRectToScene(rect);

        QQuickWindow *window = m_parentItem->window();
        if (window) {
            if (rect.top() < 0 || rect.bottom() > window->height()) {
                // if the popup doesn't fit on the screen, try flipping it around (below <-> above)
                QRectF flipped = m_parentItem->mapRectToScene(QRectF(m_x, m_parentItem->height() - m_y - rect.height(), rect.width(), rect.height()));
                if (flipped.y() >= 0 && flipped.bottom() < window->height())
                    rect = flipped;
            }
        }
    }

    m_popup->popupItem->setPosition(rect.topLeft());
}

void QQuickPopupPositioner::removeAncestorListeners(QQuickItem *item)
{
    if (item == m_parentItem)
        return;

    QQuickItem *p = item;
    while (p) {
        QQuickItemPrivate::get(p)->removeItemChangeListener(this, AncestorChangeTypes);
        p = p->parentItem();
    }
}

void QQuickPopupPositioner::addAncestorListeners(QQuickItem *item)
{
    if (item == m_parentItem)
        return;

    QQuickItem *p = item;
    while (p) {
        QQuickItemPrivate::get(p)->addItemChangeListener(this, AncestorChangeTypes);
        p = p->parentItem();
    }
}

// TODO: use QQuickItem::isAncestorOf() in dev/5.7
bool QQuickPopupPositioner::isAncestor(QQuickItem *item) const
{
    if (!m_parentItem)
        return false;

    QQuickItem *parent = m_parentItem->parentItem();
    while (parent) {
        if (parent == item)
            return true;
        parent = parent->parentItem();
    }
    return false;
}

QQuickPopupTransitionManager::QQuickPopupTransitionManager(QQuickPopupPrivate *popup)
    : QQuickTransitionManager()
    , state(Off)
    , popup(popup)
{
}

void QQuickPopupTransitionManager::transitionEnter()
{
    if (state == Enter && isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Enter;
    transition(actions, popup->enter, popup->popupItem);
}

void QQuickPopupTransitionManager::transitionExit()
{
    if (state == Exit && isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Exit;
    transition(actions, popup->exit, popup->popupItem);
}

void QQuickPopupTransitionManager::finished()
{
    if (state == Enter)
        popup->finalizeEnterTransition();
    else if (state == Exit)
        popup->finalizeExitTransition();

    state = Off;
}

QQuickPopup::QQuickPopup(QObject *parent)
    : QObject(*(new QQuickPopupPrivate), parent)
{
    Q_D(QQuickPopup);
    d->init();
}

QQuickPopup::QQuickPopup(QQuickPopupPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QQuickPopup);
    d->init();
}

QQuickPopup::~QQuickPopup()
{
    Q_D(QQuickPopup);
    d->positioner.setParentItem(Q_NULLPTR);
}

/*!
    \qmlmethod void Qt.labs.controls::Popup::open()

    Opens the popup.
*/
void QQuickPopup::open()
{
    Q_D(QQuickPopup);
    if (d->overlay) {
        // popup already open
        return;
    }

    QQuickWindow *window = Q_NULLPTR;
    QObject *p = parent();
    while (p && !window) {
        if (QQuickItem *item = qobject_cast<QQuickItem *>(p)) {
            window = item->window();
            if (!window)
                p = item->parentItem();
        } else {
            window = qobject_cast<QQuickWindow *>(p);
            if (!window)
                p = p->parent();
        }
    }
    if (!window) {
        qmlInfo(this) << "cannot find any window to open popup in.";
        return;
    }

    QQuickApplicationWindow *applicationWindow = qobject_cast<QQuickApplicationWindow*>(window);
    if (!applicationWindow) {
        // FIXME Maybe try to open it in that window somehow
        qmlInfo(this) << "is not in an ApplicationWindow.";
        return;
    }

    d->overlay = static_cast<QQuickOverlay *>(applicationWindow->overlay());
    d->popupItem->setParentItem(d->overlay);
    d->positioner.setParentItem(d->parentItem);
    // TODO: add Popup::transformOrigin?
    if (d->contentItem)
        d->popupItem->setTransformOrigin(d->contentItem->transformOrigin());
    emit aboutToShow();
    d->transitionManager.transitionEnter();
    emit visibleChanged();
}

/*!
    \qmlmethod void Qt.labs.controls::Popup::close()

    Closes the popup.
*/
void QQuickPopup::close()
{
    Q_D(QQuickPopup);
    if (!d->overlay) {
        // popup already closed
        return;
    }

    d->popupItem->setFocus(false);
    emit aboutToHide();
    d->transitionManager.transitionExit();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::x

    This property holds the x-coordinate of the popup.
*/
qreal QQuickPopup::x() const
{
    Q_D(const QQuickPopup);
    return d->positioner.x();
}

void QQuickPopup::setX(qreal x)
{
    Q_D(QQuickPopup);
    d->positioner.setX(x);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::y

    This property holds the y-coordinate of the popup.
*/
qreal QQuickPopup::y() const
{
    Q_D(const QQuickPopup);
    return d->positioner.y();
}

void QQuickPopup::setY(qreal y)
{
    Q_D(QQuickPopup);
    d->positioner.setY(y);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::width

    This property holds the width of the popup.
*/
qreal QQuickPopup::width() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->width();
}

void QQuickPopup::setWidth(qreal width)
{
    Q_D(QQuickPopup);
    d->popupItem->setWidth(width);
}

void QQuickPopup::resetWidth()
{
    Q_D(QQuickPopup);
    d->popupItem->resetWidth();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::height

    This property holds the height of the popup.
*/
qreal QQuickPopup::height() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->height();
}

void QQuickPopup::setHeight(qreal height)
{
    Q_D(QQuickPopup);
    d->popupItem->setHeight(height);
}

void QQuickPopup::resetHeight()
{
    Q_D(QQuickPopup);
    d->popupItem->resetHeight();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::implicitWidth

    This property holds the implicit width of the popup.
*/
qreal QQuickPopup::implicitWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitWidth();
}

void QQuickPopup::setImplicitWidth(qreal width)
{
    Q_D(QQuickPopup);
    d->popupItem->setImplicitWidth(width);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::implicitHeight

    This property holds the implicit height of the popup.
*/
qreal QQuickPopup::implicitHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->implicitHeight();
}

void QQuickPopup::setImplicitHeight(qreal height)
{
    Q_D(QQuickPopup);
    d->popupItem->setImplicitHeight(height);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::contentWidth

    This property holds the content width. It is used for calculating the
    total implicit width of the Popup.

    \note If only a single item is used within the Popup, the implicit width
          of its contained item is used as the content width.
*/
qreal QQuickPopup::contentWidth() const
{
    Q_D(const QQuickPopup);
    return d->contentWidth;
}

void QQuickPopup::setContentWidth(qreal width)
{
    Q_D(QQuickPopup);
    if (d->contentWidth != width) {
        d->contentWidth = width;
        emit contentWidthChanged();
    }
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::contentHeight

    This property holds the content height. It is used for calculating the
    total implicit height of the Popup.

    \note If only a single item is used within the Popup, the implicit height
          of its contained item is used as the content height.
*/
qreal QQuickPopup::contentHeight() const
{
    Q_D(const QQuickPopup);
    return d->contentHeight;
}

void QQuickPopup::setContentHeight(qreal height)
{
    Q_D(QQuickPopup);
    if (d->contentHeight != height) {
        d->contentHeight = height;
        emit contentHeightChanged();
    }
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::availableWidth

    This property holds the width available after deducting horizontal padding.

    \sa padding, leftPadding, rightPadding
*/
qreal QQuickPopup::availableWidth() const
{
    return qMax<qreal>(0.0, width() - leftPadding() - rightPadding());
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::availableHeight

    This property holds the height available after deducting vertical padding.

    \sa padding, topPadding, bottomPadding
*/
qreal QQuickPopup::availableHeight() const
{
    return qMax<qreal>(0.0, height() - topPadding() - bottomPadding());
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::padding

    This property holds the default padding.

    \sa availableWidth, availableHeight, topPadding, leftPadding, rightPadding, bottomPadding
*/
qreal QQuickPopup::padding() const
{
    Q_D(const QQuickPopup);
    return d->padding;
}

void QQuickPopup::setPadding(qreal padding)
{
    Q_D(QQuickPopup);
    if (qFuzzyCompare(d->padding, padding))
        return;
    QMarginsF oldPadding(leftPadding(), topPadding(), rightPadding(), bottomPadding());
    d->padding = padding;
    emit paddingChanged();
    QMarginsF newPadding(leftPadding(), topPadding(), rightPadding(), bottomPadding());
    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()))
        emit topPaddingChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()))
        emit leftPaddingChanged();
    if (!qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit rightPaddingChanged();
    if (!qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit bottomPaddingChanged();
    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()) || !qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit availableHeightChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()) || !qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit availableWidthChanged();
    paddingChange(newPadding, oldPadding);
}

void QQuickPopup::resetPadding()
{
    setPadding(0);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::topPadding

    This property holds the top padding.

    \sa padding, bottomPadding, availableHeight
*/
qreal QQuickPopup::topPadding() const
{
    Q_D(const QQuickPopup);
    if (d->hasTopPadding)
        return d->topPadding;
    return d->padding;
}

void QQuickPopup::setTopPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->setTopPadding(padding);
}

void QQuickPopup::resetTopPadding()
{
    Q_D(QQuickPopup);
    d->setTopPadding(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::leftPadding

    This property holds the left padding.

    \sa padding, rightPadding, availableWidth
*/
qreal QQuickPopup::leftPadding() const
{
    Q_D(const QQuickPopup);
    if (d->hasLeftPadding)
        return d->leftPadding;
    return d->padding;
}

void QQuickPopup::setLeftPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->setLeftPadding(padding);
}

void QQuickPopup::resetLeftPadding()
{
    Q_D(QQuickPopup);
    d->setLeftPadding(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::rightPadding

    This property holds the right padding.

    \sa padding, leftPadding, availableWidth
*/
qreal QQuickPopup::rightPadding() const
{
    Q_D(const QQuickPopup);
    if (d->hasRightPadding)
        return d->rightPadding;
    return d->padding;
}

void QQuickPopup::setRightPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->setRightPadding(padding);
}

void QQuickPopup::resetRightPadding()
{
    Q_D(QQuickPopup);
    d->setRightPadding(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::bottomPadding

    This property holds the bottom padding.

    \sa padding, topPadding, availableHeight
*/
qreal QQuickPopup::bottomPadding() const
{
    Q_D(const QQuickPopup);
    if (d->hasBottomPadding)
        return d->bottomPadding;
    return d->padding;
}

void QQuickPopup::setBottomPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->setBottomPadding(padding);
}

void QQuickPopup::resetBottomPadding()
{
    Q_D(QQuickPopup);
    d->setBottomPadding(0, true);
}

/*!
    \qmlproperty Item Qt.labs.popups::Popup::parent

    This property holds the parent item.
*/
QQuickItem *QQuickPopup::parentItem() const
{
    Q_D(const QQuickPopup);
    return d->parentItem;
}

void QQuickPopup::setParentItem(QQuickItem *parent)
{
    Q_D(QQuickPopup);
    if (d->parentItem != parent) {
        d->parentItem = parent;
        if (d->positioner.parentItem())
            d->positioner.setParentItem(parent);
        emit parentChanged();
    }
}

/*!
    \qmlproperty Item Qt.labs.popups::Popup::background

    This property holds the background item.

    \note If the background item has no explicit size specified, it automatically
          follows the popup's size. In most cases, there is no need to specify
          width or height for a background item.
*/
QQuickItem *QQuickPopup::background() const
{
    Q_D(const QQuickPopup);
    return d->background;
}

void QQuickPopup::setBackground(QQuickItem *background)
{
    Q_D(QQuickPopup);
    if (d->background != background) {
        delete d->background;
        d->background = background;
        if (background) {
            background->setParentItem(d->popupItem);
            if (qFuzzyIsNull(background->z()))
                background->setZ(-1);
            if (isComponentComplete())
                d->resizeBackground();
        }
        emit backgroundChanged();
    }
}

/*!
    \qmlproperty Item Qt.labs.controls::Popup::contentItem

    This property holds the content item of the popup.

    The content item is the visual implementation of the popup. When the
    popup is made visible, the content item is automatically reparented to
    the \l {ApplicationWindow::overlay}{overlay item} of its application
    window.
*/
QQuickItem *QQuickPopup::contentItem() const
{
    Q_D(const QQuickPopup);
    return d->contentItem;
}

void QQuickPopup::setContentItem(QQuickItem *item)
{
    Q_D(QQuickPopup);
    if (d->overlay) {
        // FIXME qmlInfo needs to know about QQuickItem and/or QObject
        static_cast<QDebug>(qmlInfo(this) << "cannot set content item") << item << "while Popup is visible.";
        return;
    }
    if (d->contentItem != item) {
        contentItemChange(item, d->contentItem);
        delete d->contentItem;
        d->contentItem = item;
        if (item) {
            item->setParentItem(d->popupItem);
            QQuickItemPrivate::get(item)->isTabFence = true;
            if (isComponentComplete())
                d->resizeContent();
        }
        emit contentItemChanged();
    }
}

/*!
    \qmlproperty list<Object> Qt.labs.controls::Popup::contentData
    \default

    This property holds the list of content data.

    \sa Item::data
*/
QQmlListProperty<QObject> QQuickPopup::contentData()
{
    Q_D(QQuickPopup);
    return QQmlListProperty<QObject>(d->contentItem, Q_NULLPTR,
                                     QQuickItemPrivate::data_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlproperty list<Item> Qt.labs.controls::Popup::contentChildren

    This property holds the list of content children.

    \sa Item::children
*/
QQmlListProperty<QQuickItem> QQuickPopup::contentChildren()
{
    Q_D(QQuickPopup);
    return QQmlListProperty<QQuickItem>(d->contentItem, Q_NULLPTR,
                                        QQuickItemPrivate::children_append,
                                        QQuickItemPrivate::children_count,
                                        QQuickItemPrivate::children_at,
                                        QQuickItemPrivate::children_clear);
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::focus

    This property holds whether the popup has focus.
*/
bool QQuickPopup::hasFocus() const
{
    Q_D(const QQuickPopup);
    return d->focus;
}

void QQuickPopup::setFocus(bool focus)
{
    Q_D(QQuickPopup);
    if (d->focus == focus)
        return;
    d->focus = focus;
    emit focusChanged();
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::modal

    This property holds whether the popup is modal.
*/
bool QQuickPopup::isModal() const
{
    Q_D(const QQuickPopup);
    return d->modal;
}

void QQuickPopup::setModal(bool modal)
{
    Q_D(QQuickPopup);
    if (d->modal == modal)
        return;
    d->modal = modal;
    emit modalChanged();
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::visible

    This property holds whether the popup is visible.
*/
bool QQuickPopup::isVisible() const
{
    Q_D(const QQuickPopup);
    return d->overlay != Q_NULLPTR /*&& !d->transitionManager.isRunning()*/;
}

/*!
    \qmlproperty Transition Qt.labs.controls::Popup::enter

    This property holds the transition that is applied to the content item
    when the popup is opened and enters the screen.
*/
QQuickTransition *QQuickPopup::enter() const
{
    Q_D(const QQuickPopup);
    return d->enter;
}

void QQuickPopup::setEnter(QQuickTransition *transition)
{
    Q_D(QQuickPopup);
    if (d->enter == transition)
        return;
    d->enter = transition;
    emit enterChanged();
}

/*!
    \qmlproperty Transition Qt.labs.controls::Popup::exit

    This property holds the transition that is applied to the content item
    when the popup is closed and exits the screen.
*/
QQuickTransition *QQuickPopup::exit() const
{
    Q_D(const QQuickPopup);
    return d->exit;
}

void QQuickPopup::setExit(QQuickTransition *transition)
{
    Q_D(QQuickPopup);
    if (d->exit == transition)
        return;
    d->exit = transition;
    emit exitChanged();
}

/*!
    \qmlproperty list<Object> Qt.labs.controls::Popup::data
    \default

    This property holds the list of data.

    \sa Item::data
*/
QQmlListProperty<QObject> QQuickPopup::data()
{
    Q_D(QQuickPopup);
    return QQuickItemPrivate::get(d->popupItem)->data();
}

void QQuickPopup::classBegin()
{
}

void QQuickPopup::componentComplete()
{
    Q_D(QQuickPopup);
    d->complete = true;
    if (!parentItem())
        setParentItem(qobject_cast<QQuickItem *>(parent()));
}

bool QQuickPopup::isComponentComplete() const
{
    Q_D(const QQuickPopup);
    return d->complete;
}

void QQuickPopup::focusInEvent(QFocusEvent *event)
{
    event->accept();
}

void QQuickPopup::focusOutEvent(QFocusEvent *event)
{
    event->accept();
}

void QQuickPopup::keyPressEvent(QKeyEvent *event)
{
    event->accept();
}

void QQuickPopup::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
}

void QQuickPopup::mousePressEvent(QMouseEvent *event)
{
    event->accept();
}

void QQuickPopup::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

void QQuickPopup::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void QQuickPopup::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
}

void QQuickPopup::mouseUngrabEvent()
{
}

void QQuickPopup::wheelEvent(QWheelEvent *event)
{
    event->accept();
}

void QQuickPopup::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_UNUSED(newItem);
    Q_UNUSED(oldItem);
}

void QQuickPopup::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPopup);
    d->resizeBackground();
    d->resizeContent();
    if (!qFuzzyCompare(newGeometry.width(), oldGeometry.width())) {
        emit widthChanged();
        emit availableWidthChanged();
    }
    if (!qFuzzyCompare(newGeometry.height(), oldGeometry.height())) {
        emit heightChanged();
        emit availableHeightChanged();
    }
}

void QQuickPopup::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickPopup);
    Q_UNUSED(newPadding);
    Q_UNUSED(oldPadding);
    d->resizeContent();
}

QT_END_NAMESPACE

#include "moc_qquickpopup_p.cpp"
