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
#include "qquickcontrol_p_p.h"

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

    Popup is the base type of popup-like user interface controls. It can be
    used with Window or ApplicationWindow.

    \qml
    import QtQuick.Window 2.2
    import Qt.labs.controls 1.0

    Window {
        id: window
        width: 400
        height: 400
        visible: true

        Button {
            text: "Open"
            onClicked: popup.open()
        }

        Popup {
            id: popup
            x: 100
            y: 100
            width: 200
            height: 300
            modal: true
            focus: true
            closePolicy: Popup.OnEscape | Popup.OnPressOutside
        }
    }
    \endqml

    In order to ensure that a popup is displayed above other items in the
    scene, it is recommended to use ApplicationWindow. ApplicationWindow also
    provides background dimming effects.

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
    , complete(false)
    , hasTopMargin(false)
    , hasLeftMargin(false)
    , hasRightMargin(false)
    , hasBottomMargin(false)
    , margins(0)
    , topMargin(0)
    , leftMargin(0)
    , rightMargin(0)
    , bottomMargin(0)
    , contentWidth(0)
    , contentHeight(0)
    , closePolicy(QQuickPopup::OnEscape)
    , parentItem(Q_NULLPTR)
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
    q->setParentItem(qobject_cast<QQuickItem *>(parent));
    QObject::connect(popupItem, &QQuickControl::paddingChanged, q, &QQuickPopup::paddingChanged);
}

bool QQuickPopupPrivate::tryClose(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickPopup);
    const bool isPress = event->type() == QEvent::MouseButtonPress;
    const bool onOutside = closePolicy.testFlag(isPress ? QQuickPopup::OnPressOutside : QQuickPopup::OnReleaseOutside);
    const bool onOutsideParent = closePolicy.testFlag(isPress ? QQuickPopup::OnPressOutsideParent : QQuickPopup::OnReleaseOutsideParent);
    if (onOutside || onOutsideParent) {
        if (onOutsideParent) {
            if (!popupItem->contains(item->mapToItem(popupItem, event->pos())) &&
                    (!parentItem || !parentItem->contains(item->mapToItem(parentItem, event->pos())))) {
                q->close();
                return true;
            }
        } else if (onOutside) {
            if (!popupItem->contains(item->mapToItem(popupItem, event->pos()))) {
                q->close();
                return true;
            }
        }
    }
    return false;
}

void QQuickPopupPrivate::finalizeEnterTransition()
{
    if (focus)
        popupItem->setFocus(true);
}

void QQuickPopupPrivate::finalizeExitTransition()
{
    positioner.setParentItem(Q_NULLPTR);
    popupItem->setParentItem(Q_NULLPTR);
    popupItem->setVisible(false);
}

QMarginsF QQuickPopupPrivate::getMargins() const
{
    Q_Q(const QQuickPopup);
    return QMarginsF(q->leftMargin(), q->topMargin(), q->rightMargin(), q->bottomMargin());
}

void QQuickPopupPrivate::setTopMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->topMargin();
    topMargin = value;
    hasTopMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->topMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(leftMargin, oldMargin, rightMargin, bottomMargin));
    }
}

void QQuickPopupPrivate::setLeftMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->leftMargin();
    leftMargin = value;
    hasLeftMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->leftMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(oldMargin, topMargin, rightMargin, bottomMargin));
    }
}

void QQuickPopupPrivate::setRightMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->rightMargin();
    rightMargin = value;
    hasRightMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->rightMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(leftMargin, topMargin, oldMargin, bottomMargin));
    }
}

void QQuickPopupPrivate::setBottomMargin(qreal value, bool reset)
{
    Q_Q(QQuickPopup);
    qreal oldMargin = q->bottomMargin();
    bottomMargin = value;
    hasBottomMargin = !reset;
    if ((!reset && !qFuzzyCompare(oldMargin, value)) || (reset && !qFuzzyCompare(oldMargin, margins))) {
        emit q->bottomMarginChanged();
        q->marginsChange(QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin),
                         QMarginsF(leftMargin, topMargin, rightMargin, oldMargin));
    }
}

class QQuickPopupItemPrivate : public QQuickControlPrivate
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
    isTabFence = true;
}

void QQuickPopupItemPrivate::implicitWidthChanged()
{
    QQuickControlPrivate::implicitWidthChanged();
    emit popup->implicitWidthChanged();
}

void QQuickPopupItemPrivate::implicitHeightChanged()
{
    QQuickControlPrivate::implicitHeightChanged();
    emit popup->implicitHeightChanged();
}

QQuickPopupItem::QQuickPopupItem(QQuickPopup *popup) :
    QQuickControl(*(new QQuickPopupItemPrivate(popup)), Q_NULLPTR)
{
    setParent(popup);
    setVisible(false);
    setFlag(ItemIsFocusScope);
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

void QQuickPopupItem::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickPopupItem);
    QQuickControl::contentItemChange(newItem, oldItem);
    d->popup->contentItemChange(newItem, oldItem);
}

void QQuickPopupItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPopupItem);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    d->popup->geometryChanged(newGeometry, oldGeometry);
}

void QQuickPopupItem::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickPopupItem);
    QQuickControl::itemChange(change, data);
    switch (change) {
    case ItemVisibleHasChanged:
        emit d->popup->visibleChanged();
        break;
    case ItemActiveFocusHasChanged:
        emit d->popup->activeFocusChanged();
        break;
    case ItemOpacityHasChanged:
        emit d->popup->opacityChanged();
        break;
    default:
        break;
    }
}

void QQuickPopupItem::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickPopupItem);
    QQuickControl::paddingChange(newPadding, oldPadding);
    d->popup->paddingChange(newPadding, oldPadding);
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickPopupItem::accessibleRole() const
{
    Q_D(const QQuickPopupItem);
    return d->popup->accessibleRole();
}
#endif // QT_NO_ACCESSIBILITY

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
        if (m_popup->popupItem->isVisible())
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
        if (m_popup->popupItem->isVisible())
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

    if (m_popup->popupItem->isVisible())
        repositionPopup();
}

void QQuickPopupPositioner::itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &)
{
    if (m_popup->popupItem->isVisible())
        repositionPopup();
}

void QQuickPopupPositioner::itemParentChanged(QQuickItem *, QQuickItem *parent)
{
    addAncestorListeners(parent);
}

void QQuickPopupPositioner::itemChildRemoved(QQuickItem *item, QQuickItem *child)
{
    if (isAncestor(child))
        removeAncestorListeners(item);
}

void QQuickPopupPositioner::itemDestroyed(QQuickItem *item)
{
    Q_ASSERT(m_parentItem == item);

    m_parentItem = Q_NULLPTR;
    m_popup->parentItem = Q_NULLPTR;
    QQuickItemPrivate::get(item)->removeItemChangeListener(this, ItemChangeTypes);
    removeAncestorListeners(item->parentItem());
}

void QQuickPopupPositioner::repositionPopup()
{
    const qreal w = m_popup->popupItem->width();
    const qreal h = m_popup->popupItem->height();
    const qreal iw = m_popup->popupItem->implicitWidth();
    const qreal ih = m_popup->popupItem->implicitHeight();

    bool adjusted = false;
    QRectF rect(m_x, m_y, iw > 0 ? iw : w, ih > 0 ? ih : h);
    if (m_parentItem) {
        rect = m_parentItem->mapRectToScene(rect);

        QQuickWindow *window = m_parentItem->window();
        if (window) {
            const QMarginsF margins = m_popup->getMargins();
            const QRectF bounds = QRectF(0, 0, window->width(), window->height()).marginsRemoved(margins);

            // push inside the margins
            if (margins.top() > 0 && rect.top() < bounds.top())
                rect.moveTop(margins.top());
            if (margins.bottom() > 0 && rect.bottom() > bounds.bottom())
                rect.moveBottom(bounds.bottom());
            if (margins.left() > 0 && rect.left() < bounds.left())
                rect.moveLeft(margins.left());
            if (margins.right() > 0 && rect.right() > bounds.right())
                rect.moveRight(bounds.right());

            if (rect.top() < bounds.top() || rect.bottom() > bounds.bottom()) {
                // if the popup doesn't fit inside the window, try flipping it around (below <-> above)
                const QRectF flipped = m_parentItem->mapRectToScene(QRectF(m_x, m_parentItem->height() - m_y - rect.height(), rect.width(), rect.height()));
                if (flipped.top() >= bounds.top() && flipped.bottom() < bounds.bottom()) {
                    adjusted = true;
                    rect = flipped;
                } else if (ih > 0) {
                    // neither the flipped around geometry fits inside the window, choose
                    // whichever side (above vs. below) fits larger part of the popup
                    const QRectF primary = rect.intersected(bounds);
                    const QRectF secondary = flipped.intersected(bounds);

                    if (primary.height() > secondary.height()) {
                        rect.setY(primary.y());
                        rect.setHeight(primary.height());
                    } else {
                        rect.setY(secondary.y());
                        rect.setHeight(secondary.height());
                    }
                    adjusted = true;
                }
            }
        }
    }

    m_popup->popupItem->setPosition(rect.topLeft());
    if (adjusted && ih > 0)
        m_popup->popupItem->setHeight(rect.height());
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

    QQuickItem *parent = m_parentItem;
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
    delete d->popupItem;
}

/*!
    \qmlmethod void Qt.labs.controls::Popup::open()

    Opens the popup.
*/
void QQuickPopup::open()
{
    Q_D(QQuickPopup);
    if (d->popupItem->isVisible())
        return;

    QQuickWindow *window = Q_NULLPTR;
    if (d->parentItem)
        window = d->parentItem->window();
    if (!window) {
        qmlInfo(this) << "cannot find any window to open popup in.";
        return;
    }

    QQuickApplicationWindow *applicationWindow = qobject_cast<QQuickApplicationWindow*>(window);
    if (!applicationWindow) {
        window->installEventFilter(this);
        d->popupItem->setZ(10001); // DefaultWindowDecoration+1
        d->popupItem->setParentItem(window->contentItem());
    } else {
        d->popupItem->setParentItem(applicationWindow->overlay());
    }

    emit aboutToShow();
    d->popupItem->setVisible(true);
    d->positioner.setParentItem(d->parentItem);
    d->transitionManager.transitionEnter();
}

/*!
    \qmlmethod void Qt.labs.controls::Popup::close()

    Closes the popup.
*/
void QQuickPopup::close()
{
    Q_D(QQuickPopup);
    if (!d->popupItem->isVisible())
        return;

    if (d->parentItem) {
        QQuickWindow *window = d->parentItem->window();
        if (!qobject_cast<QQuickApplicationWindow *>(window)) {
            window->removeEventFilter(this);
        }
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
    \qmlproperty real Qt.labs.controls::Popup::z

    This property holds the z-value of the popup. Z-value determines
    the stacking order of popups. The default z-value is \c 0.
*/
qreal QQuickPopup::z() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->z();
}

void QQuickPopup::setZ(qreal z)
{
    Q_D(QQuickPopup);
    if (qFuzzyCompare(z, d->popupItem->z()))
        return;
    d->popupItem->setZ(z);
    emit zChanged();
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
    \readonly

    This property holds the width available after deducting horizontal padding.

    \sa padding, leftPadding, rightPadding
*/
qreal QQuickPopup::availableWidth() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->availableWidth();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::availableHeight
    \readonly

    This property holds the height available after deducting vertical padding.

    \sa padding, topPadding, bottomPadding
*/
qreal QQuickPopup::availableHeight() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->availableHeight();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::margins

    This property holds the default margins around the popup.

    \sa topMargin, leftMargin, rightMargin, bottomMargin
*/
qreal QQuickPopup::margins() const
{
    Q_D(const QQuickPopup);
    return d->margins;
}

void QQuickPopup::setMargins(qreal margins)
{
    Q_D(QQuickPopup);
    if (qFuzzyCompare(d->margins, margins))
        return;
    QMarginsF oldMargins(leftMargin(), topMargin(), rightMargin(), bottomMargin());
    d->margins = margins;
    emit marginsChanged();
    QMarginsF newMargins(leftMargin(), topMargin(), rightMargin(), bottomMargin());
    if (!qFuzzyCompare(newMargins.top(), oldMargins.top()))
        emit topMarginChanged();
    if (!qFuzzyCompare(newMargins.left(), oldMargins.left()))
        emit leftMarginChanged();
    if (!qFuzzyCompare(newMargins.right(), oldMargins.right()))
        emit rightMarginChanged();
    if (!qFuzzyCompare(newMargins.bottom(), oldMargins.bottom()))
        emit bottomMarginChanged();
    marginsChange(newMargins, oldMargins);
}

void QQuickPopup::resetMargins()
{
    setMargins(0);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::topMargin

    This property holds the top margin around the popup.

    \sa margins, bottomMargin
*/
qreal QQuickPopup::topMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasTopMargin)
        return d->topMargin;
    return d->margins;
}

void QQuickPopup::setTopMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setTopMargin(margin);
}

void QQuickPopup::resetTopMargin()
{
    Q_D(QQuickPopup);
    d->setTopMargin(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::leftMargin

    This property holds the left margin around the popup.

    \sa margins, rightMargin
*/
qreal QQuickPopup::leftMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasLeftMargin)
        return d->leftMargin;
    return d->margins;
}

void QQuickPopup::setLeftMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setLeftMargin(margin);
}

void QQuickPopup::resetLeftMargin()
{
    Q_D(QQuickPopup);
    d->setLeftMargin(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::rightMargin

    This property holds the right margin around the popup.

    \sa margins, leftMargin
*/
qreal QQuickPopup::rightMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasRightMargin)
        return d->rightMargin;
    return d->margins;
}

void QQuickPopup::setRightMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setRightMargin(margin);
}

void QQuickPopup::resetRightMargin()
{
    Q_D(QQuickPopup);
    d->setRightMargin(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::bottomMargin

    This property holds the bottom margin around the popup.

    \sa margins, topMargin
*/
qreal QQuickPopup::bottomMargin() const
{
    Q_D(const QQuickPopup);
    if (d->hasBottomMargin)
        return d->bottomMargin;
    return d->margins;
}

void QQuickPopup::setBottomMargin(qreal margin)
{
    Q_D(QQuickPopup);
    d->setBottomMargin(margin);
}

void QQuickPopup::resetBottomMargin()
{
    Q_D(QQuickPopup);
    d->setBottomMargin(0, true);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::padding

    This property holds the default padding.

    \sa availableWidth, availableHeight, topPadding, leftPadding, rightPadding, bottomPadding
*/
qreal QQuickPopup::padding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->padding();
}

void QQuickPopup::setPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setPadding(padding);
}

void QQuickPopup::resetPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetPadding();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::topPadding

    This property holds the top padding.

    \sa padding, bottomPadding, availableHeight
*/
qreal QQuickPopup::topPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->topPadding();
}

void QQuickPopup::setTopPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setTopPadding(padding);
}

void QQuickPopup::resetTopPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetTopPadding();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::leftPadding

    This property holds the left padding.

    \sa padding, rightPadding, availableWidth
*/
qreal QQuickPopup::leftPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->leftPadding();
}

void QQuickPopup::setLeftPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setLeftPadding(padding);
}

void QQuickPopup::resetLeftPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetLeftPadding();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::rightPadding

    This property holds the right padding.

    \sa padding, leftPadding, availableWidth
*/
qreal QQuickPopup::rightPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->rightPadding();
}

void QQuickPopup::setRightPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setRightPadding(padding);
}

void QQuickPopup::resetRightPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetRightPadding();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::bottomPadding

    This property holds the bottom padding.

    \sa padding, topPadding, availableHeight
*/
qreal QQuickPopup::bottomPadding() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->bottomPadding();
}

void QQuickPopup::setBottomPadding(qreal padding)
{
    Q_D(QQuickPopup);
    d->popupItem->setBottomPadding(padding);
}

void QQuickPopup::resetBottomPadding()
{
    Q_D(QQuickPopup);
    d->popupItem->resetBottomPadding();
}

QQuickItem *QQuickPopup::popupItem() const
{
    Q_D(const QQuickPopup);
    return d->popupItem;
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
    return d->popupItem->background();
}

void QQuickPopup::setBackground(QQuickItem *background)
{
    Q_D(QQuickPopup);
    if (d->popupItem->background() == background)
        return;

    d->popupItem->setBackground(background);
    emit backgroundChanged();
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
    return d->popupItem->contentItem();
}

void QQuickPopup::setContentItem(QQuickItem *item)
{
    Q_D(QQuickPopup);
    d->popupItem->setContentItem(item);
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
    return QQmlListProperty<QObject>(d->popupItem->contentItem(), Q_NULLPTR,
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
    return QQmlListProperty<QQuickItem>(d->popupItem->contentItem(), Q_NULLPTR,
                                        QQuickItemPrivate::children_append,
                                        QQuickItemPrivate::children_count,
                                        QQuickItemPrivate::children_at,
                                        QQuickItemPrivate::children_clear);
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::clip

    This property holds whether clipping is enabled. The default value is \c false.
*/
bool QQuickPopup::clip() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->clip();
}

void QQuickPopup::setClip(bool clip)
{
    Q_D(QQuickPopup);
    if (clip == d->popupItem->clip())
        return;
    d->popupItem->setClip(clip);
    emit clipChanged();
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::focus

    This property holds whether the popup has focus. The default value is \c false.
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
    \qmlproperty bool Qt.labs.controls::Popup::activeFocus
    \readonly

    This property holds whether the popup has active focus.
*/
bool QQuickPopup::hasActiveFocus() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->hasActiveFocus();
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::modal

    This property holds whether the popup is modal. The default value is \c false.
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

    This property holds whether the popup is visible. The default value is \c false.
*/
bool QQuickPopup::isVisible() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->isVisible();
}

void QQuickPopup::setVisible(bool visible)
{
    if (visible)
        open();
    else
        close();
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::opacity

    This property holds the opacity of the popup. The default value is \c 1.0.
*/
qreal QQuickPopup::opacity() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->opacity();
}

void QQuickPopup::setOpacity(qreal opacity)
{
    Q_D(QQuickPopup);
    d->popupItem->setOpacity(opacity);
}

/*!
    \qmlproperty real Qt.labs.controls::Popup::scale

    This property holds the scale factor of the popup. The default value is \c 1.0.
*/
qreal QQuickPopup::scale() const
{
    Q_D(const QQuickPopup);
    return d->popupItem->scale();
}

void QQuickPopup::setScale(qreal scale)
{
    Q_D(QQuickPopup);
    if (qFuzzyCompare(scale, d->popupItem->scale()))
        return;
    d->popupItem->setScale(scale);
    emit scaleChanged();
}

/*!
    \qmlproperty enumeration Qt.labs.controls::Popup::closePolicy

    This property determines the circumstances under which the popup closes.
    The flags can be combined to allow several ways of closing the popup.

    The available values are:
    \value Popup.NoAutoClose The popup will only close when manually instructed to do so.
    \value Popup.OnPressOutside The popup will close when the mouse is pressed outside of it.
    \value Popup.OnPressOutsideParent The popup will close when the mouse is pressed outside of its parent.
    \value Popup.OnReleaseOutside The popup will close when the mouse is released outside of it.
    \value Popup.OnReleaseOutsideParent The popup will close when the mouse is released outside of its parent.
    \value Popup.OnEscape The popup will close when the escape key is pressed while the popup
        has active focus.

    The default value is \c Popup.OnEscape.
*/
QQuickPopup::ClosePolicy QQuickPopup::closePolicy() const
{
    Q_D(const QQuickPopup);
    return d->closePolicy;
}

void QQuickPopup::setClosePolicy(ClosePolicy policy)
{
    Q_D(QQuickPopup);
    if (d->closePolicy == policy)
        return;
    d->closePolicy = policy;
    emit closePolicyChanged();
}

/*!
    \qmlproperty enumeration Qt.labs.controls::Popup::transformOrigin

    This property holds the origin point for transformations in enter and exit transitions.

    Nine transform origins are available, as shown in the image below.
    The default transform origin is \c Popup.Center.

    \image qtlabscontrols-popup-transformorigin.png

    \sa enter, exit, Item::transformOrigin
*/
QQuickPopup::TransformOrigin QQuickPopup::transformOrigin() const
{
    Q_D(const QQuickPopup);
    return static_cast<TransformOrigin>(d->popupItem->transformOrigin());
}

void QQuickPopup::setTransformOrigin(TransformOrigin origin)
{
    Q_D(QQuickPopup);
    d->popupItem->setTransformOrigin(static_cast<QQuickItem::TransformOrigin>(origin));
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

bool QQuickPopup::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickPopup);
    Q_UNUSED(object);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        if (d->modal)
            event->setAccepted(true);
        if (QQuickWindow *window = qobject_cast<QQuickWindow *>(object)) {
            if (d->tryClose(window->contentItem(), static_cast<QMouseEvent *>(event)))
                return true;
        }
        return false;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseMove:
    case QEvent::Wheel:
        if (d->modal)
            event->setAccepted(true);
        return false;
    default:
        return false;
    }
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
    Q_D(QQuickPopup);
    event->accept();

    if (event->key() != Qt::Key_Escape)
        return;

    if (d->closePolicy.testFlag(OnEscape))
        close();
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
    emit contentItemChanged();
}

void QQuickPopup::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPopup);
    d->positioner.repositionPopup();
    if (!qFuzzyCompare(newGeometry.width(), oldGeometry.width())) {
        emit widthChanged();
        emit availableWidthChanged();
    }
    if (!qFuzzyCompare(newGeometry.height(), oldGeometry.height())) {
        emit heightChanged();
        emit availableHeightChanged();
    }
}

void QQuickPopup::marginsChange(const QMarginsF &newMargins, const QMarginsF &oldMargins)
{
    Q_D(QQuickPopup);
    Q_UNUSED(newMargins);
    Q_UNUSED(oldMargins);
    d->positioner.repositionPopup();
}

void QQuickPopup::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    const bool tp = !qFuzzyCompare(newPadding.top(), oldPadding.top());
    const bool lp = !qFuzzyCompare(newPadding.left(), oldPadding.left());
    const bool rp = !qFuzzyCompare(newPadding.right(), oldPadding.right());
    const bool bp = !qFuzzyCompare(newPadding.bottom(), oldPadding.bottom());

    if (tp)
        emit topPaddingChanged();
    if (lp)
        emit leftPaddingChanged();
    if (rp)
        emit rightPaddingChanged();
    if (bp)
        emit bottomPaddingChanged();

    if (lp || rp)
        emit availableWidthChanged();
    if (tp || bp)
        emit availableHeightChanged();
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickPopup::accessibleRole() const
{
    return QAccessible::LayeredPane;
}
#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#include "moc_qquickpopup_p.cpp"
